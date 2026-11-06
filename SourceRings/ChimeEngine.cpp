#include "ChimeEngine.h"

#include <algorithm>
#include <cmath>

#include "Parameters.h"

namespace rng
{

using namespace rings;

namespace
{
constexpr int kPrimeFrames = 80;
}

ChimeEngine::ChimeEngine()
    : part_ (std::make_unique<Part>()), strummer_ (std::make_unique<Strummer>())
{
    reverbBuffer_.resize (32768, 0);
    part_->Init (reverbBuffer_.data());
    strummer_->Init (0.01f, kSampleRate / (float) kBlockFrames);
}

ChimeEngine::~ChimeEngine() = default;

void ChimeEngine::prepare (double hostSampleRate, int maxBlockSize)
{
    hostRate_ = hostSampleRate > 0.0 ? hostSampleRate : 48000.0;
    const int maxBlock = std::max (kBlockFrames, maxBlockSize);

    down_.prepare (hostRate_, (double) kSampleRate, 2, maxBlock);
    up_.prepare ((double) kSampleRate, hostRate_, 2, maxBlock);

    const int cap = (int) std::ceil ((double) maxBlock * (double) kSampleRate / hostRate_) + 64;
    for (int ch = 0; ch < 2; ++ch)
    {
        d48_[(size_t) ch].assign ((size_t) cap, 0.0f);
        q48_[(size_t) ch].assign ((size_t) (cap + kBlockFrames * 2), 0.0f);
        pend_[(size_t) ch].assign ((size_t) kBlockFrames, 0.0f);
    }

    reset();
}

void ChimeEngine::reset()
{
    down_.reset();
    up_.reset();
    pendCount_ = 0;
    qCount_ = 0;

    for (int ch = 0; ch < 2; ++ch)
        std::fill (pend_[(size_t) ch].begin(), pend_[(size_t) ch].end(), 0.0f);

    if ((int) d48_[0].size() >= kPrimeFrames)
    {
        std::fill (d48_[0].begin(), d48_[0].end(), 0.0f);
        const float* z[2] = { d48_[0].data(), d48_[0].data() };
        float* sink[2] = { d48_[1].data(), d48_[1].data() };
        up_.process (z, kPrimeFrames, sink, 0);
    }
}

void ChimeEngine::setModel (int index)
{
    index = std::clamp (index, 0, (int) RESONATOR_MODEL_LAST - 1);
    part_->set_model (static_cast<ResonatorModel> (index));
}

void ChimeEngine::setPolyphony (int voices)
{
    part_->set_polyphony (std::clamp (voices, 1, (int) kMaxPolyphony));
}

void ChimeEngine::noteOn (int midiNote)
{
    heldNote_ = midiNote;
    noteOnPending_ = true;
}

void ChimeEngine::noteOff()
{
    heldNote_ = -1;
    noteOffPending_ = false; // pitch just goes free-running again next block
}

void ChimeEngine::runBlock()
{
    PerformanceState ps {};
    Patch patch {};

    // ATTENUVERT macro from cv_scaler.cc: pot + attenuverter(law) * cv,
    // constrained. With cv == 0 (nothing automated) this reduces to the
    // plain pot value, same as nothing patched into the CV jack.
    auto attenuvert = [] (float pot, float amountKnob, float cv, float (*law) (float), float lo, float hi)
    {
        float v = pot + law (amountKnob) * cv;
        return std::clamp (v, lo, hi);
    };

    patch.structure = attenuvert (structure_, structureAmount_, structureCv_, quadraticBipolarLaw, 0.0f, 0.9995f);
    patch.brightness = attenuvert (brightness_, brightnessAmount_, brightnessCv_, quadraticBipolarLaw, 0.0f, 1.0f);
    patch.damping = attenuvert (damping_, dampingAmount_, dampingCv_, quadraticBipolarLaw, 0.0f, 1.0f);
    patch.position = attenuvert (position_, positionAmount_, positionCv_, quadraticBipolarLaw, 0.0f, 1.0f);

    // Pitch FM: hardware slew-limits big jumps (fm_cv_ tracking in
    // cv_scaler.cc) to avoid zipper noise; replicated verbatim.
    const float fmTarget = frequencyCv_ * 48.0f;
    const float error = fmTarget - fmSlew_;
    if (std::abs (error) >= 0.8f)
        fmSlew_ = fmTarget;
    else
        fmSlew_ += 0.02f * error;
    ps.fm = std::clamp (fmSlew_ * quarticBipolarLaw (frequencyAmount_), -48.0f, 48.0f);

    // Frequency knob: semitone-quantized transpose with hysteresis,
    // matching the firmware's ADC_CHANNEL_POT_FREQUENCY handling.
    const float transposeTarget = 60.0f * frequency_;
    const float hysteresis = transposeTarget - transpose_ > 0.0f ? -0.3f : 0.3f;
    transpose_ = (float) (int) (transposeTarget + hysteresis + 0.5f);

    const bool hasNote = heldNote_ >= 0;
    ps.internal_note = ! hasNote; // no MIDI note held == nothing patched into V/OCT
    if (hasNote)
    {
        // C3 (MIDI 60) -> 0 semitones offset, matching a typical 1V/oct
        // calibration reference; tonic snaps to the Frequency knob exactly
        // like hardware's quantized transpose when something is patched.
        ps.note = (float) (heldNote_ - 60);
        ps.tonic = 12.0f + transpose_;
    }
    else
    {
        ps.note = 0.0f;
        ps.tonic = 12.0f + transposeTarget; // unquantized, free-running
    }

    ps.internal_exciter = false; // host audio is always a real exciter signal
    switch (strumMode_)
    {
        case StrumMode::externalTrigger:
            ps.internal_strum = false;
            ps.strum = noteOnPending_;
            break;
        case StrumMode::noteChange:
        case StrumMode::audioOnset:
            ps.internal_strum = true;
            ps.strum = false; // Strummer fills this in below
            break;
    }
    noteOnPending_ = false;

    strummer_->Process (pend_[0].data(), (size_t) kBlockFrames, &ps);

    float* out[2] = { q48_[0].data() + qCount_, q48_[1].data() + qCount_ };
    part_->Process (ps, patch, pend_[0].data(), out[0], out[1], (size_t) kBlockFrames);
    qCount_ += kBlockFrames;

    previousNote_ = ps.note;
}

void ChimeEngine::process (float* left, float* right, int numSamples)
{
    if (numSamples <= 0)
        return;

    const float* in[2] = { left, right };
    float* d48[2] = { d48_[0].data(), d48_[1].data() };
    const int n48 = down_.process (in, numSamples, d48, (int) d48_[0].size());

    qCount_ = 0;
    for (int i = 0; i < n48; ++i)
    {
        // Rings takes a single mono exciter signal; sum to mono like the
        // hardware's single IN jack.
        pend_[0][(size_t) pendCount_] = 0.5f * (d48[0][i] + d48[1][i]);
        pend_[1][(size_t) pendCount_] = pend_[0][(size_t) pendCount_];

        if (++pendCount_ == kBlockFrames)
        {
            runBlock();
            pendCount_ = 0;
        }
    }

    const float* q[2] = { q48_[0].data(), q48_[1].data() };
    float* out[2] = { left, right };
    const int produced = up_.process (q, qCount_, out, numSamples);

    for (int i = produced; i < numSamples; ++i)
    {
        left[i] = produced > 0 ? left[produced - 1] : 0.0f;
        right[i] = produced > 0 ? right[produced - 1] : 0.0f;
    }
}

} // namespace rng

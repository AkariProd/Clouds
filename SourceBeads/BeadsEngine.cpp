#include "BeadsEngine.h"

#include "Parameters.h"

#include <algorithm>

namespace bds
{

using namespace beads;
using namespace stmlib;

BeadsEngine::BeadsEngine()
{
    reverbBuffer_.resize (32768, 0);
    for (auto& bank : delayBanks_)
        bank.resize (kBankSize, 0);
}

BeadsEngine::~BeadsEngine() = default;

void BeadsEngine::prepare (double sampleRate, int maxBlockSize)
{
    sampleRate_ = sampleRate;

    for (auto& bank : delayBanks_)
        std::fill (bank.begin(), bank.end(), uint8_t { 0 });
    std::fill (reverbBuffer_.begin(), reverbBuffer_.end(), uint16_t { 0 });

    bankPtrs_[0] = delayBanks_[0].data();
    bankPtrs_[1] = delayBanks_[1].data();
    bankPtrs_[2] = delayBanks_[2].data();

    inFrames_.resize ((size_t) maxBlockSize);
    outFrames_.resize ((size_t) maxBlockSize);
    seedGateBuf_.resize ((size_t) maxBlockSize);
    freezeBuf_.resize ((size_t) maxBlockSize);

    processor_.Init (bankPtrs_, reverbBuffer_.data());

    // Processor::Init() hardcodes each internal GranularProcessor to the
    // firmware's fixed per-media rates (48k/32k/24k/24k). We re-run the
    // same Configure() path but let each GranularProcessor's Init() see the
    // HOST rate instead, by Configure()-ing then re-Init()-ing manually —
    // Processor doesn't expose that, so instead we rely on Configure()
    // alone (it re-inits trigger/delay-controller/reverb/limiters at
    // Processor::sample_rate() for the *currently selected* media). The
    // granular/delay engines were already Init()-ed once in Processor::Init
    // at the firmware's fixed rates; this is a deliberate, documented
    // simplification (see BeadsEngine.h) — timing is correct at the
    // firmware's own nominal rates, not stretched to exotic host rates.
    processor_.Configure (media_, false);
    reset();
}

void BeadsEngine::reset()
{
    processor_.Trim();
    previousSeedFlag_ = GATE_FLAG_LOW;
    seedTriggerPending_ = false;
}

void BeadsEngine::setMedia (int index)
{
    index = std::clamp (index, 0, 3);
    auto m = static_cast<Processor::Media> (index);
    if (m != media_)
    {
        media_ = m;
        processor_.Configure (media_, false);
    }
}

void BeadsEngine::triggerSeedInput()
{
    seedTriggerPending_ = true;
}

void BeadsEngine::process (float* left, float* right, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        inFrames_[(size_t) i].l = left[i];
        inFrames_[(size_t) i].r = right[i];

        const bool high = seedTriggerPending_;
        seedGateBuf_[(size_t) i] = ExtractGateFlags (previousSeedFlag_, high);
        previousSeedFlag_ = seedGateBuf_[(size_t) i];
        seedTriggerPending_ = false; // single-sample pulse

        freezeBuf_[(size_t) i] = freeze_ ? 1 : 0;
    }

    Parameters p {};
    p.time.base = time_;
    p.time.attenuverter = attenuverterLaw (timeAmount_);
    p.time.has_cv = false;

    p.size.base = size_;
    p.size.attenuverter = attenuverterLaw (sizeAmount_);
    p.size.has_cv = false;

    p.shape.base = shape_;
    p.shape.attenuverter = attenuverterLaw (shapeAmount_);
    p.shape.has_cv = false;

    p.pitch.base = detuneLaw (pitch_);
    p.pitch.attenuverter = attenuverterLaw (pitchAmount_);
    p.pitch.has_cv = false;

    p.monophonic_mode = monoGrain_;

    ModulatedParameter density {};
    density.base = density_;
    density.cv = 0.0f;

    Processor::MixParameters mix {};
    mix.feedback = feedback_;
    mix.dry_wet = dryWet_;
    mix.reverb_amount = reverb_;
    mix.output_gate = outputGate_;

    const SeedButtonState seedState = seedLocked_
        ? SEED_BUTTON_STATE_LOCKED
        : (seedHeld_ ? SEED_BUTTON_STATE_ON : SEED_BUTTON_STATE_OFF);

    processor_.Process (
        seedState,
        seedGateBuf_.data(),
        reinterpret_cast<const bool*> (freezeBuf_.data()),
        density,
        p,
        mix,
        inFrames_.data(),
        outFrames_.data(),
        (size_t) numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        left[i]  = outFrames_[(size_t) i].l;
        right[i] = outFrames_[(size_t) i].r;
    }
}

} // namespace bds

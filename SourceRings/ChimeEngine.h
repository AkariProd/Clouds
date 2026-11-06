#pragma once

#include <array>
#include <memory>
#include <vector>
#include <cstdint>

#include "rings/dsp/part.h"
#include "rings/dsp/strummer.h"
#include "rings/dsp/patch.h"
#include "rings/dsp/performance_state.h"

#include "Resampler.h"

namespace rng
{

enum class StrumMode { externalTrigger = 0, noteChange = 1, audioOnset = 2 };

/** Wraps rings::Part + rings::Strummer (the real firmware resonator engine
    and strum-trigger logic, vendored verbatim from eurorack/rings/dsp).

    rings::kSampleRate (48 kHz) is a compile-time global baked into every
    filter/oscillator in the DSP, not a runtime parameter like Beads'
    per-media rate — so unlike Pebble, this can't just run at the host's
    rate. It's bridged with the same Kaiser-windowed sinc resampler Clouds
    uses (Resampler.h): host rate <-> 48 kHz in, Part::Process() called in
    the firmware's own kMaxBlockSize=24 chunks, 48 kHz <-> host rate out. */
class ChimeEngine
{
public:
    ChimeEngine();
    ~ChimeEngine();

    void prepare (double hostSampleRate, int maxBlockSize);
    void reset();

    void process (float* left, float* right, int numSamples);

    void setFrequency (float v)       { frequency_ = v; }
    void setFrequencyAmount (float v) { frequencyAmount_ = v; }
    void setFrequencyCv (float v)     { frequencyCv_ = v; }
    void setStructure (float v)       { structure_ = v; }
    void setStructureAmount (float v) { structureAmount_ = v; }
    void setStructureCv (float v)     { structureCv_ = v; }
    void setBrightness (float v)      { brightness_ = v; }
    void setBrightnessAmount (float v){ brightnessAmount_ = v; }
    void setBrightnessCv (float v)    { brightnessCv_ = v; }
    void setDamping (float v)         { damping_ = v; }
    void setDampingAmount (float v)   { dampingAmount_ = v; }
    void setDampingCv (float v)       { dampingCv_ = v; }
    void setPosition (float v)        { position_ = v; }
    void setPositionAmount (float v)  { positionAmount_ = v; }
    void setPositionCv (float v)      { positionCv_ = v; }

    void setModel (int index);
    void setPolyphony (int voices);
    void setStrumMode (StrumMode m) { strumMode_ = m; }

    /** Current MIDI note (-1 = none held): drives V/OCT, exactly like
        patching a V/OCT source on hardware. No note held = free pitch from
        the Frequency knob alone, matching nothing patched into V/OCT. */
    void noteOn (int midiNote);
    void noteOff();

private:
    void runBlock();

    std::unique_ptr<rings::Part> part_;
    std::unique_ptr<rings::Strummer> strummer_;

    std::vector<uint16_t> reverbBuffer_;

    cld::SincResampler down_, up_;
    std::array<std::vector<float>, 2> d48_, q48_, pend_;
    int pendCount_ = 0, qCount_ = 0;

    static constexpr int kBlockFrames = (int) rings::kMaxBlockSize; // 24

    float frequency_ = 0.5f, frequencyAmount_ = 0.5f, frequencyCv_ = 0.0f;
    float structure_ = 0.5f, structureAmount_ = 0.5f, structureCv_ = 0.0f;
    float brightness_ = 0.5f, brightnessAmount_ = 0.5f, brightnessCv_ = 0.0f;
    float damping_ = 0.5f, dampingAmount_ = 0.5f, dampingCv_ = 0.0f;
    float position_ = 0.5f, positionAmount_ = 0.5f, positionCv_ = 0.0f;

    StrumMode strumMode_ = StrumMode::externalTrigger;

    int heldNote_ = -1;
    bool noteOnPending_ = false, noteOffPending_ = false;
    float previousNote_ = 0.0f;
    float transpose_ = 0.0f;
    float fmSlew_ = 0.0f;

    double hostRate_ = 48000.0;
};

} // namespace rng

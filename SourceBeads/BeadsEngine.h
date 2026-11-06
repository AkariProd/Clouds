#pragma once

#include <array>
#include <vector>
#include <cstdint>

#include "beads/dsp/processor.h"
#include "beads/dsp/frame.h"
#include "stmlib/utils/gate_flags.h"

namespace bds
{

/** Wraps beads::Processor (the real firmware granular/delay/media/reverb
    chain, vendored verbatim from eurorack/beads/dsp) behind a simple
    block-based interface for a JUCE AudioProcessor.

    Hardware's physical trick of changing the ADC/DAC's actual sample rate
    per "media" setting (48k digital / 32k vintage digital / 24k tape) can't
    be replicated in a VST host, which picks one fixed sample rate for the
    whole session. Each of the four internal beads::GranularProcessor
    instances is instead initialised at the HOST's sample rate directly, so
    the granular/delay/reverb timing is correct at any host rate. The
    bit-depth / mu-law quantization character that defines each media
    setting's lo-fi colour is untouched and fully faithful — only the
    tape-rate band-limiting side-effect is not reproduced. */
class BeadsEngine
{
public:
    BeadsEngine();
    ~BeadsEngine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** In-place stereo processing. buffer must have exactly 2 channels. */
    void process (float* left, float* right, int numSamples);

    void setTime (float v)        { time_ = v; }
    void setTimeAmount (float v)  { timeAmount_ = v; }
    void setDensity (float v)     { density_ = v; }
    void setPitch (float v)       { pitch_ = v; }
    void setPitchAmount (float v) { pitchAmount_ = v; }
    void setSize (float v)        { size_ = v; }
    void setSizeAmount (float v)  { sizeAmount_ = v; }
    void setShape (float v)       { shape_ = v; }
    void setShapeAmount (float v) { shapeAmount_ = v; }

    void setFeedback (float v) { feedback_ = v; }
    void setDryWet (float v)   { dryWet_ = v; }
    void setReverb (float v)   { reverb_ = v; }

    void setFreeze (bool v)     { freeze_ = v; }
    void setSeed (bool v)       { seedHeld_ = v; }
    void setSeedLock (bool v)   { seedLocked_ = v; }
    void setMonoGrain (bool v)  { monoGrain_ = v; }
    void setOutputGate (bool v) { outputGate_ = v; }

    /** 0 = Dirty Tape, 1 = Clean Tape, 2 = Vintage Digital, 3 = Digital —
        same order as beads::Processor::Media. */
    void setMedia (int index);

    /** Edge-triggered: call once per host MIDI note-on. Produces a single-
        sample rising gate on the (otherwise permanently low / unpatched)
        SEED CV input, the same jack the front-panel SEED button logic
        reads alongside. */
    void triggerSeedInput();

private:
    beads::Processor processor_;

    static constexpr size_t kBankSize = 262144; // 256 KiB, matches firmware.
    std::array<std::vector<uint8_t>, 3> delayBanks_;
    std::vector<uint16_t> reverbBuffer_;
    uint8_t* bankPtrs_[3] {};

    std::vector<beads::FloatFrame> inFrames_;
    std::vector<beads::FloatFrame> outFrames_;
    std::vector<stmlib::GateFlags> seedGateBuf_;
    std::vector<uint8_t> freezeBuf_; // 0/1, read back as bool* (sizeof(bool)==1 here)

    beads::Processor::Media media_ = beads::Processor::MEDIA_DIGITAL;

    float time_ = 0.5f, timeAmount_ = 0.5f;
    float density_ = 0.5f;
    float pitch_ = 0.5f, pitchAmount_ = 0.5f;
    float size_ = 0.5f, sizeAmount_ = 0.5f;
    float shape_ = 0.5f, shapeAmount_ = 0.5f;
    float feedback_ = 0.0f, dryWet_ = 0.5f, reverb_ = 0.0f;
    bool freeze_ = false;
    bool seedHeld_ = false;
    bool seedLocked_ = false;
    bool monoGrain_ = false;
    bool outputGate_ = false;

    stmlib::GateFlags previousSeedFlag_ = stmlib::GATE_FLAG_LOW;
    bool seedTriggerPending_ = false;

    double sampleRate_ = 48000.0;
};

} // namespace bds

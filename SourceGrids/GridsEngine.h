#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>

#include "grids/pattern_generator.h"

namespace grd
{

// ---------------------------------------------------------------------------
// Built-in analog-style drum voices.
//
// Grids' hardware only ever produced TRIG/ACC gate pulses meant to drive
// separate VCA/VCO modules downstream -- it has no sound of its own. Since
// this port runs standalone with no modular rack behind it, each of the
// three parts gets a small original synthesis voice here (not ported from
// any Mutable Instruments source -- this part is original code written for
// this plugin) so the instrument makes sound on its own.
// ---------------------------------------------------------------------------

class KickVoice
{
public:
    void prepare (double sampleRate) { sampleRate_ = sampleRate; }
    void trigger (bool accent)
    {
        phase_ = 0.0;
        pitchEnv_ = 1.0f;
        ampEnv_ = 1.0f;
        clickEnv_ = accent ? 1.0f : 0.6f;
        velocity_ = accent ? 1.0f : 0.72f;
        active_ = true;
    }
    float process()
    {
        if (! active_)
            return 0.0f;

        const float freq = 42.0f + pitchEnv_ * 110.0f;
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        const float tone = std::sin (phase_ * juce::MathConstants<float>::twoPi);

        const float click = clickEnv_ * (((float) rand() / (float) RAND_MAX) * 2.0f - 1.0f);

        pitchEnv_ *= 0.9985f;
        ampEnv_ *= 0.9975f;
        clickEnv_ *= 0.90f;

        if (ampEnv_ < 0.0005f)
            active_ = false;

        return velocity_ * (tone * ampEnv_ * 0.9f + click * 0.5f);
    }

private:
    double sampleRate_ = 48000.0;
    double phase_ = 0.0;
    float pitchEnv_ = 0.0f, ampEnv_ = 0.0f, clickEnv_ = 0.0f, velocity_ = 1.0f;
    bool active_ = false;
};

class SnareVoice
{
public:
    void prepare (double sampleRate) { sampleRate_ = sampleRate; hp_ = 0.0f; }
    void trigger (bool accent)
    {
        phase_ = 0.0;
        toneEnv_ = 1.0f;
        noiseEnv_ = 1.0f;
        velocity_ = accent ? 1.0f : 0.68f;
        active_ = true;
    }
    float process()
    {
        if (! active_)
            return 0.0f;

        phase_ += 190.0 / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        const float tone = std::sin (phase_ * juce::MathConstants<float>::twoPi);

        const float white = ((float) rand() / (float) RAND_MAX) * 2.0f - 1.0f;
        hp_ = 0.6f * (hp_ + white - prevWhite_);
        prevWhite_ = white;

        toneEnv_ *= 0.994f;
        noiseEnv_ *= 0.9935f;

        if (toneEnv_ < 0.0005f && noiseEnv_ < 0.0005f)
            active_ = false;

        return velocity_ * (tone * toneEnv_ * 0.35f + hp_ * noiseEnv_ * 0.8f);
    }

private:
    double sampleRate_ = 48000.0;
    double phase_ = 0.0;
    float toneEnv_ = 0.0f, noiseEnv_ = 0.0f, velocity_ = 1.0f;
    float hp_ = 0.0f, prevWhite_ = 0.0f;
    bool active_ = false;
};

class HatVoice
{
public:
    void prepare (double sampleRate) { sampleRate_ = sampleRate; hp_ = 0.0f; }
    void trigger (bool accent)
    {
        for (auto& p : phase_) p = 0.0;
        env_ = 1.0f;
        velocity_ = accent ? 1.0f : 0.6f;
        active_ = true;
    }
    float process()
    {
        if (! active_)
            return 0.0f;

        static const float ratios[6] = { 1.0f, 1.342f, 1.753f, 2.0f, 2.414f, 2.853f };
        float sum = 0.0f;
        for (int i = 0; i < 6; ++i)
        {
            phase_[(size_t) i] += (520.0 * ratios[i]) / sampleRate_;
            if (phase_[(size_t) i] >= 1.0) phase_[(size_t) i] -= 1.0;
            sum += phase_[(size_t) i] < 0.5 ? 1.0f : -1.0f;
        }
        sum /= 6.0f;

        hp_ = 0.85f * (hp_ + sum - prevSum_);
        prevSum_ = sum;

        env_ *= 0.978f;
        if (env_ < 0.0008f)
            active_ = false;

        return velocity_ * hp_ * env_ * 0.6f;
    }

private:
    double sampleRate_ = 48000.0;
    std::array<double, 6> phase_ {};
    float env_ = 0.0f, velocity_ = 1.0f;
    float hp_ = 0.0f, prevSum_ = 0.0f;
    bool active_ = false;
};

// ---------------------------------------------------------------------------

enum class GridsMode { drums = 0, euclidean = 1 };

/** Wraps grids::PatternGenerator (the real firmware topographic
    drum-pattern engine, vendored verbatim from eurorack/grids) and drives
    it from the host's own tempo/transport instead of a hardware CLOCK
    jack, feeding its trigger+accent output into three built-in drum
    voices instead of external TRIG/ACC gate outputs. */
class GridsEngine
{
public:
    GridsEngine();

    void prepare (double hostSampleRate);
    void reset();

    void process (float* left, float* right, int numSamples, juce::AudioPlayHead* playHead);

    void setMapX (float v01) { mapX_ = v01; }
    void setMapY (float v01) { mapY_ = v01; }
    void setChaos (float v01) { chaos_ = v01; }
    void setBdDensity (float v01) { bdDensity_ = v01; }
    void setSdDensity (float v01) { sdDensity_ = v01; }
    void setHhDensity (float v01) { hhDensity_ = v01; }
    void setSwing (bool on) { swing_ = on; }
    void setMode (GridsMode m) { mode_ = m; }
    void setEuclideanLength (int part, float v01) { euclideanLength_[(size_t) part] = v01; }

    void requestReset() { resetRequested_ = true; }

    /** Per-part activity, for the editor's trigger LEDs (0=BD,1=SD,2=HH). */
    bool partActive (int part) const { return ledState_[(size_t) part]; }

private:
    void onStep();

    grids::PatternGenerator pattern_;

    KickVoice kick_;
    SnareVoice snare_;
    HatVoice hat_;

    double sampleRate_ = 48000.0;
    double songPpq_ = 0.0;
    double basePpq_ = 0.0;
    bool wasPlaying_ = false;
    int lastStepIndex_ = -1;

    bool resetRequested_ = false;

    float mapX_ = 0.5f, mapY_ = 0.5f, chaos_ = 0.0f;
    float bdDensity_ = 0.6f, sdDensity_ = 0.5f, hhDensity_ = 0.5f;
    bool swing_ = false;
    GridsMode mode_ = GridsMode::drums;
    std::array<float, 3> euclideanLength_ { 0.5f, 0.5f, 0.5f };

    std::array<bool, 3> ledState_ { false, false, false };
    int ledHoldSamples_ = 0;
    static constexpr int kLedHoldMax = 2400;
};

} // namespace grd

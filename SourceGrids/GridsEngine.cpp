#include "GridsEngine.h"

namespace grd
{

GridsEngine::GridsEngine()
{
    pattern_.Init();

    auto* settings = pattern_.mutable_settings();
    settings->options.drums.x = 128;
    settings->options.drums.y = 128;
    settings->options.drums.randomness = 0;
    settings->density[0] = 160;
    settings->density[1] = 128;
    settings->density[2] = 128;

    pattern_.set_output_mode (grids::OUTPUT_MODE_DRUMS);
    pattern_.set_output_clock (false);  // use the accent-bits-in-high-nibble layout
    pattern_.set_gate_mode (false);     // short trigger pulses, not held gates
    pattern_.set_swing (false);
}

void GridsEngine::prepare (double hostSampleRate)
{
    sampleRate_ = hostSampleRate > 0.0 ? hostSampleRate : 48000.0;
    kick_.prepare (sampleRate_);
    snare_.prepare (sampleRate_);
    hat_.prepare (sampleRate_);
    reset();
}

void GridsEngine::reset()
{
    pattern_.Reset();
    songPpq_ = 0.0;
    basePpq_ = 0.0;
    lastStepIndex_ = -1;
    wasPlaying_ = false;
}

void GridsEngine::onStep()
{
    auto* settings = pattern_.mutable_settings();

    if (mode_ == GridsMode::drums)
    {
        settings->options.drums.x = (uint8_t) juce::jlimit (0, 255, (int) (mapX_ * 255.0f));
        settings->options.drums.y = (uint8_t) juce::jlimit (0, 255, (int) (mapY_ * 255.0f));
        settings->options.drums.randomness = (uint8_t) juce::jlimit (0, 255, (int) (chaos_ * 255.0f));
        pattern_.set_output_mode (grids::OUTPUT_MODE_DRUMS);
    }
    else
    {
        settings->options.euclidean_length[0] = (uint8_t) juce::jlimit (0, 255, (int) (euclideanLength_[0] * 255.0f));
        settings->options.euclidean_length[1] = (uint8_t) juce::jlimit (0, 255, (int) (euclideanLength_[1] * 255.0f));
        settings->options.euclidean_length[2] = (uint8_t) juce::jlimit (0, 255, (int) (euclideanLength_[2] * 255.0f));
        pattern_.set_output_mode (grids::OUTPUT_MODE_EUCLIDEAN);
    }
    settings->density[0] = (uint8_t) juce::jlimit (0, 255, (int) (bdDensity_ * 255.0f));
    settings->density[1] = (uint8_t) juce::jlimit (0, 255, (int) (sdDensity_ * 255.0f));
    settings->density[2] = (uint8_t) juce::jlimit (0, 255, (int) (hhDensity_ * 255.0f));
    pattern_.set_swing (swing_);

    pattern_.TickClock (grids::kPulsesPerStep);

    const uint8_t state = pattern_.state();
    const bool bdTrig = (state & 0x01) != 0, sdTrig = (state & 0x02) != 0, hhTrig = (state & 0x04) != 0;
    const bool bdAcc = (state & 0x08) != 0, sdAcc = (state & 0x10) != 0, hhAcc = (state & 0x20) != 0;

    if (bdTrig) { kick_.trigger (bdAcc); ledState_[0] = true; }
    if (sdTrig) { snare_.trigger (sdAcc); ledState_[1] = true; }
    if (hhTrig) { hat_.trigger (hhAcc); ledState_[2] = true; }

    if (bdTrig || sdTrig || hhTrig)
        ledHoldSamples_ = kLedHoldMax;
}

void GridsEngine::process (float* left, float* right, int numSamples, juce::AudioPlayHead* playHead)
{
    double bpm = 120.0;
    bool isPlaying = true;
    bool havePosition = false;

    if (playHead != nullptr)
    {
        if (auto pos = playHead->getPosition())
        {
            if (auto hostBpm = pos->getBpm())
                bpm = *hostBpm > 1.0 ? *hostBpm : 120.0;
            isPlaying = pos->getIsPlaying();
            if (auto ppq = pos->getPpqPosition())
            {
                songPpq_ = *ppq;
                havePosition = true;
            }
        }
    }

    if (resetRequested_)
    {
        basePpq_ = songPpq_;
        pattern_.Reset();
        lastStepIndex_ = -1;
        resetRequested_ = false;
    }

    const bool running = havePosition ? isPlaying : true; // standalone/no-transport: free-run
    const double ppqPerSample = bpm / 60.0 / sampleRate_;
    const float swingFrac = 0.0f; // computed per-step below when swing_ is on

    for (int i = 0; i < numSamples; ++i)
    {
        if (running)
        {
            if (! havePosition)
                songPpq_ += ppqPerSample;

            const int stepIndex = (int) std::floor ((songPpq_ - basePpq_) * 8.0);
            if (stepIndex != lastStepIndex_)
            {
                lastStepIndex_ = stepIndex;
                onStep();
            }
        }

        const float sample = kick_.process() + snare_.process() + hat_.process();
        left[i] = sample;
        right[i] = sample;

        if (ledHoldSamples_ > 0 && --ledHoldSamples_ == 0)
            ledState_ = { false, false, false };
    }

    juce::ignoreUnused (swingFrac);
}

} // namespace grd

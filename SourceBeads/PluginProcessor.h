#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#include "BeadsEngine.h"
#include "Parameters.h"

/** Everything the panel animates, published from the audio thread. */
struct PebblePanelState
{
    std::atomic<float> envIn  { 0.0f };
    std::atomic<float> envOut { 0.0f };
    std::atomic<float> inPeak { 0.0f };
};

class PebbleProcessor final : public juce::AudioProcessor
{
public:
    PebbleProcessor();
    ~PebbleProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Pebble"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    PebblePanelState panel;

private:
    void pullParameters();

    bds::BeadsEngine engine_;

    std::atomic<float>* time_ = nullptr;
    std::atomic<float>* timeAmount_ = nullptr;
    std::atomic<float>* density_ = nullptr;
    std::atomic<float>* pitch_ = nullptr;
    std::atomic<float>* pitchAmount_ = nullptr;
    std::atomic<float>* size_ = nullptr;
    std::atomic<float>* sizeAmount_ = nullptr;
    std::atomic<float>* shape_ = nullptr;
    std::atomic<float>* shapeAmount_ = nullptr;
    std::atomic<float>* feedback_ = nullptr;
    std::atomic<float>* dryWet_ = nullptr;
    std::atomic<float>* reverb_ = nullptr;
    std::atomic<float>* freeze_ = nullptr;
    std::atomic<float>* seed_ = nullptr;
    std::atomic<float>* seedLock_ = nullptr;
    std::atomic<float>* media_ = nullptr;
    std::atomic<float>* monoGrain_ = nullptr;
    std::atomic<float>* outputGate_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PebbleProcessor)
};

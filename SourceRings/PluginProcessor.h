#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "ChimeEngine.h"
#include "Parameters.h"

class ChimeProcessor final : public juce::AudioProcessor
{
public:
    ChimeProcessor();
    ~ChimeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Chime"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    void pullParameters();

    rng::ChimeEngine engine_;

    std::atomic<float>* frequency_ = nullptr;
    std::atomic<float>* frequencyAmount_ = nullptr;
    std::atomic<float>* frequencyCv_ = nullptr;
    std::atomic<float>* structure_ = nullptr;
    std::atomic<float>* structureAmount_ = nullptr;
    std::atomic<float>* structureCv_ = nullptr;
    std::atomic<float>* brightness_ = nullptr;
    std::atomic<float>* brightnessAmount_ = nullptr;
    std::atomic<float>* brightnessCv_ = nullptr;
    std::atomic<float>* damping_ = nullptr;
    std::atomic<float>* dampingAmount_ = nullptr;
    std::atomic<float>* dampingCv_ = nullptr;
    std::atomic<float>* position_ = nullptr;
    std::atomic<float>* positionAmount_ = nullptr;
    std::atomic<float>* positionCv_ = nullptr;
    std::atomic<float>* model_ = nullptr;
    std::atomic<float>* polyphony_ = nullptr;
    std::atomic<float>* strumMode_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChimeProcessor)
};

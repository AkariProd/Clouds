#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "GridsEngine.h"
#include "Parameters.h"

class ContourProcessor final : public juce::AudioProcessor
{
public:
    ContourProcessor();
    ~ContourProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Contour"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void requestReset() { engine_.requestReset(); }
    bool partActive (int part) const { return engine_.partActive (part); }

    juce::AudioProcessorValueTreeState apvts;

private:
    void pullParameters();

    grd::GridsEngine engine_;

    std::atomic<float>* mapX_ = nullptr;
    std::atomic<float>* mapY_ = nullptr;
    std::atomic<float>* chaos_ = nullptr;
    std::atomic<float>* bdDensity_ = nullptr;
    std::atomic<float>* sdDensity_ = nullptr;
    std::atomic<float>* hhDensity_ = nullptr;
    std::atomic<float>* swing_ = nullptr;
    std::atomic<float>* mode_ = nullptr;
    std::atomic<float>* euclidBd_ = nullptr;
    std::atomic<float>* euclidSd_ = nullptr;
    std::atomic<float>* euclidHh_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContourProcessor)
};

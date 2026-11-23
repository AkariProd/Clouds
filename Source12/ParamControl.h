#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "CategorizedCombo.h"

// One knob/toggle/combo control + its label, bound to an APVTS parameter.
// Reused identically for every one of the 246 generated parameters -- the
// panel is data-driven from Parameters.cpp rather than 246 hand-placed
// components.
class ParamControl : public juce::Component, private juce::Timer
{
public:
    ParamControl(juce::AudioProcessorValueTreeState& apvts, const p12::ParamDef& def);

    void resized() override;
    void paint(juce::Graphics&) override;

    static constexpr int kWidth = 74;
    static constexpr int kHeight = 78;

private:
    void timerCallback() override;  // only started for the categorized-combo case

    juce::AudioProcessorValueTreeState& apvtsRef;
    const p12::ParamDef& paramDef;
    juce::Label nameLabel;

    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::ToggleButton> toggle;
    std::unique_ptr<juce::ComboBox> combo;
    std::unique_ptr<CategorizedCombo> treeCombo;  // used instead of `combo` when def.treeCategorized

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAttachment;
};

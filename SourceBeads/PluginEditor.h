#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

/** Panel layout mirrors the hardware's control grid (not a faceplate
    photo — see the branch notes): five direct knobs on top (Time, Density,
    Pitch, Size, Shape), their four attenuverter "amount" pots underneath
    (Density has none on hardware either), the three mix knobs (Feedback,
    Dry/Wet, Reverb) below that, then Freeze / Seed / Seed Lock buttons, the
    Media 4-way switch, and the two extra toggles (Mono Grain, Output Gate)
    that round out the firmware's feature set. */
class PebbleEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PebbleEditor (PebbleProcessor&);
    ~PebbleEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void makeKnob (Knob& k, const juce::String& paramId, const juce::String& text);

    PebbleProcessor& proc_;

    Knob time_, density_, pitch_, size_, shape_;
    Knob timeAmount_, pitchAmount_, sizeAmount_, shapeAmount_;
    Knob feedback_, dryWet_, reverb_;

    juce::ToggleButton freezeBtn_ { "Freeze" };
    juce::ToggleButton seedBtn_ { "Seed" };
    juce::ToggleButton seedLockBtn_ { "Seed Lock" };
    juce::ToggleButton monoGrainBtn_ { "Mono Grain" };
    juce::ToggleButton outputGateBtn_ { "Output Gate" };

    juce::ComboBox mediaBox_;
    juce::Label mediaLabel_ { {}, "Media" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAtt_, seedAtt_, seedLockAtt_, monoGrainAtt_, outputGateAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mediaAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PebbleEditor)
};

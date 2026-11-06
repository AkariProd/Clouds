#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "ChimeLookAndFeel.h"

/** Panel recreates the hardware's dark control layout — five knobs in a
    pentagon-ish arrangement (Frequency top, Structure/Brightness either
    side, Damping/Position below), a ring-shaped motif echoing the module's
    own circular graphic, and the model/polyphony/strum controls grouped
    below. Branded "Chime" / "SoulMade", per Mutable Instruments' own
    derivative-works guidance — their CC-BY-SA panel layout and colour
    scheme are recreated, their trademarked name and logo are not. */
class ChimeEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ChimeEditor (ChimeProcessor&);
    ~ChimeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void makeKnob (Knob& k, const juce::String& paramId, const juce::String& text, juce::Colour accent = {});

    ChimeProcessor& proc_;
    rng::ChimeLookAndFeel lnf_;

    Knob frequency_, structure_, brightness_, damping_, position_;
    Knob frequencyAmount_, structureAmount_, brightnessAmount_, dampingAmount_, positionAmount_;

    juce::ComboBox modelBox_, strumBox_;
    juce::Label modelLabel_ { {}, "MODEL" }, strumLabel_ { {}, "STRUM" }, polyLabel_ { {}, "POLY" };
    juce::Slider polySlider_ { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAtt_, strumAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> polyAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChimeEditor)
};

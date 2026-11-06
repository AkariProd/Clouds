#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "ContourLookAndFeel.h"

/** Panel recreates Grids' real layout: a big round SWING button in the
    panel's CLOCK-knob slot (host transport replaces the hardware CLOCK
    jack, so the slot is repurposed rather than left dead), RESET/TAP as
    one functional reset button, MAP X / MAP Y / CHAOS white-top knobs
    down the left side, three small cyan "FILL" (per-part density) knobs
    with live trigger LEDs on the right. No decorative TRIG/ACC jack grid
    -- a VST has no patch cables, so fake jacks that look clickable but
    aren't were confusing rather than genuinely faithful. Branded
    "Contour" / "SoulMade" -- "topographic drum sequencer" -> contour
    lines on a topographic map. */
class ContourEditor final : public juce::AudioProcessorEditor,
                             private juce::Timer
{
public:
    explicit ContourEditor (ContourProcessor&);
    ~ContourEditor() override;

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
    void timerCallback() override;

    ContourProcessor& proc_;
    grd::ContourLookAndFeel lnf_;

    Knob mapX_, mapY_, chaos_;
    Knob bdFill_, sdFill_, hhFill_;
    Knob euclidBd_, euclidSd_, euclidHh_;

    juce::ToggleButton swingButton_ { "SWING" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> swingAtt_;

    juce::TextButton resetButton_ { "TAP" };

    juce::ComboBox modeBox_;
    juce::Label modeLabel_ { {}, "MODE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt_;

    juce::Rectangle<int> ledArea_[3];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContourEditor)
};

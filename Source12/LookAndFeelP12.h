#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Flat-cap knob with a pointer line and a thin value arc -- deliberately NOT
// JUCE's stock ringed/filled-arc rotary, so the panel reads as an original
// synth-editor skin rather than a generic plugin. Original design, not a
// copy of any vendor's panel graphics.
class LookAndFeelP12 : public juce::LookAndFeel_V4
{
public:
    LookAndFeelP12();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;

    static const juce::Colour panelBg;
    static const juce::Colour panelBgDark;
    static const juce::Colour sectionHeaderBg;
    static const juce::Colour accent;
    static const juce::Colour knobCap;
    static const juce::Colour knobRing;
    static const juce::Colour textColour;
};

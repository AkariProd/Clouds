#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace rng
{

/** Rings' hardware panel is black with white silkscreen and light-grey
    knob caps — the inverse of Beads' white panel. Pointer-line knob style
    (no JUCE ring/arc) to match, same accent-ring idea as Pebble for the
    panel's own colour-coded marks. */
class ChimeLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ChimeLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff555555));
        setColour (juce::Label::textColourId, juce::Colours::white);
        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a2a2a));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff666666));
        setColour (juce::ToggleButton::textColourId, juce::Colours::white);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff2a2a2a));
        setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    }

    void setKnobAccent (juce::Slider& s, juce::Colour c) { s.getProperties().set ("accent", (juce::int64) c.getARGB()); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                            float sliderPosProportional, float rotaryStartAngle,
                            float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        if (slider.getProperties().contains ("accent"))
        {
            const auto accent = juce::Colour ((juce::uint32) (juce::int64) slider.getProperties()["accent"]);
            juce::Path arc;
            const float arcRadius = radius + 3.0f;
            arc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (accent.withAlpha (0.55f));
            g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            juce::Path arcValue;
            arcValue.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
            g.setColour (accent);
            g.strokePath (arcValue, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Light-grey cap (hardware's knobs read light against the black panel).
        g.setColour (juce::Colour (0xffd9d9d4));
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour (juce::Colour (0xff1a1a1a));
        g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        juce::Path pointer;
        const float pointerLength = radius * 0.78f;
        const float pointerThickness = juce::jmax (2.0f, radius * 0.12f);
        pointer.addRoundedRectangle (-pointerThickness * 0.5f, -radius * 0.92f, pointerThickness, pointerLength, pointerThickness * 0.4f);
        g.setColour (juce::Colour (0xff1a1a1a));
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto circle = bounds.withSizeKeepingCentre (
            juce::jmin (bounds.getWidth(), (float) button.getHeight()) - 2.0f,
            juce::jmin (bounds.getWidth(), (float) button.getHeight()) - 2.0f);

        g.setColour (button.getToggleState() ? juce::Colour (0xff35c9ad) : juce::Colour (0xff444444));
        g.fillEllipse (circle);
        g.setColour (juce::Colour (0xff888888));
        g.drawEllipse (circle, 1.5f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (juce::FontOptions (11.0f)).boldened());
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centredBottom);
    }
};

} // namespace rng

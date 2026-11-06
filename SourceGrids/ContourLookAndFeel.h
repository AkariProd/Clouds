#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace grd
{

/** Same light-cream/black-skirt-white-cap knob family as Chime/Pebble,
    matching Grids' real panel. */
class ContourLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ContourLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xff1a1a1a));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xffb5b5ac));
        setColour (juce::Label::textColourId, juce::Colour (0xff1a1a1a));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xff1a1a1a));
        setColour (juce::ComboBox::backgroundColourId, juce::Colours::white);
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff8a8a82));
        setColour (juce::ToggleButton::textColourId, juce::Colour (0xff1a1a1a));
        setColour (juce::TextButton::buttonColourId, juce::Colours::white);
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xff1a1a1a));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colours::white);
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xff1a1a1a));
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
            const float arcRadius = radius + 4.0f;
            arc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (accent.withAlpha (0.45f));
            g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            juce::Path arcValue;
            arcValue.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
            g.setColour (accent);
            g.strokePath (arcValue, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setColour (juce::Colour (0xff1c1c1c));
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        for (int i = 0; i < 24; ++i)
        {
            const float a = (float) i / 24.0f * juce::MathConstants<float>::twoPi;
            juce::Line<float> notch (centre.getPointOnCircumference (radius * 0.94f, a),
                                      centre.getPointOnCircumference (radius, a));
            g.setColour (juce::Colour (0xff0c0c0c));
            g.drawLine (notch, 0.6f);
        }

        const float capRadius = radius * 0.74f;
        g.setColour (juce::Colours::white);
        g.fillEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);
        g.setColour (juce::Colour (0xffd8d8d2));
        g.drawEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.0f);

        juce::Path pointer;
        const float pointerLength = capRadius * 0.85f;
        const float pointerThickness = juce::jmax (1.6f, radius * 0.1f);
        pointer.addRoundedRectangle (-pointerThickness * 0.5f, -capRadius * 0.9f, pointerThickness, pointerLength, pointerThickness * 0.4f);
        g.setColour (juce::Colour (0xff1a1a1a));
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool) override
    {
        // Big round illuminated button, repurposing the panel's CLOCK knob
        // slot (SWING) and matching TAP's round button style.
        auto bounds = button.getLocalBounds().toFloat().reduced (3.0f);
        auto circle = bounds.withSizeKeepingCentre (
            juce::jmin (bounds.getWidth(), bounds.getHeight()),
            juce::jmin (bounds.getWidth(), bounds.getHeight()));

        g.setColour (button.getToggleState() ? juce::Colour (0xffd6316f) : juce::Colours::white);
        g.fillEllipse (circle);
        g.setColour (juce::Colour (0xff8a8a82));
        g.drawEllipse (circle, 1.5f);

        g.setColour (button.getToggleState() ? juce::Colours::white : juce::Colour (0xff1a1a1a));
        g.setFont (juce::Font (juce::FontOptions (11.0f)).boldened());
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
        auto circle = bounds.withSizeKeepingCentre (
            juce::jmin (bounds.getWidth(), bounds.getHeight()),
            juce::jmin (bounds.getWidth(), bounds.getHeight()));

        juce::Colour fill = juce::Colours::white;
        if (isButtonDown) fill = juce::Colour (0xffe6e4de);
        else if (isMouseOverButton) fill = juce::Colour (0xfff0efea);

        g.setColour (fill);
        g.fillEllipse (circle);
        g.setColour (juce::Colour (0xff8a8a82));
        g.drawEllipse (circle, 1.5f);
    }
};

} // namespace grd

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bds
{

/** Flat black knob with a white pointer line, on the module's own white
    panel — not JUCE's default ringed/filled-arc rotary style. Mirrors the
    hardware's actual knob appearance (plain black cap, no value arc). */
class PebbleLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    PebbleLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::black);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff888888));
        setColour (juce::Label::textColourId, juce::Colours::black);
        setColour (juce::ComboBox::textColourId, juce::Colours::black);
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff888888));
        setColour (juce::ToggleButton::textColourId, juce::Colours::black);
    }

    /** Per-slider accent colour for the thin modulation-style ring behind the
        knob cap — the module's panel uses small colour-coded arcs/dashes
        (cyan / magenta / amber) next to groups of controls. Unset (alpha 0)
        draws no ring. Looked up by component ID, set via setKnobAccent(). */
    void setKnobAccent (juce::Slider& s, juce::Colour c) { s.getProperties().set ("accent", (juce::int64) c.getARGB()); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                            float sliderPosProportional, float rotaryStartAngle,
                            float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Colour-accented arc behind the cap, same idea as the panel's small
        // cyan/magenta/amber indicator marks next to groups of controls.
        if (slider.getProperties().contains ("accent"))
        {
            const auto accent = juce::Colour ((juce::uint32) (juce::int64) slider.getProperties()["accent"]);
            juce::Path arc;
            const float arcRadius = radius + 3.0f;
            arc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (accent);
            g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            juce::Path arcValue;
            arcValue.addCentredArc (centre.x, centre.y, arcRadius, arcRadius,
                                     0.0f, rotaryStartAngle, angle, true);
            g.setColour (accent.brighter (0.3f));
            g.strokePath (arcValue, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Cap.
        g.setColour (juce::Colour (0xff1a1a1a));
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        // Thin rim so it reads against the white panel.
        g.setColour (juce::Colour (0xff444444));
        g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        // Pointer.
        juce::Path pointer;
        const float pointerLength = radius * 0.78f;
        const float pointerThickness = juce::jmax (2.0f, radius * 0.12f);
        pointer.addRoundedRectangle (-pointerThickness * 0.5f, -radius * 0.92f, pointerThickness, pointerLength, pointerThickness * 0.4f);
        g.setColour (juce::Colours::white);
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                            bool, bool) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto circle = bounds.removeFromTop (bounds.getHeight()).withSizeKeepingCentre (
            juce::jmin (bounds.getWidth(), (float) button.getHeight()) - 2.0f,
            juce::jmin (bounds.getWidth(), (float) button.getHeight()) - 2.0f);

        g.setColour (button.getToggleState() ? juce::Colour (0xffe0442c) : juce::Colour (0xffd8d8d4));
        g.fillEllipse (circle);
        g.setColour (juce::Colour (0xff444444));
        g.drawEllipse (circle, 1.5f);

        g.setColour (juce::Colours::black);
        g.setFont (juce::Font (juce::FontOptions (11.0f)).boldened());
        g.drawText (button.getButtonText(), button.getLocalBounds(),
                    juce::Justification::centredBottom);
    }
};

} // namespace bds

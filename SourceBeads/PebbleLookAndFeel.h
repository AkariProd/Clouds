#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bds
{

/** Real 3D-shaded rendering matching the hardware photo: glossy white
    dome knobs/buttons with a radial highlight and a soft drop shadow,
    black knurled-rim knobs with their own highlight, and rounded colour
    pill switches with an icon + LED. Everything here is literal shading
    (gradients + shadows), not flat vector fills. */
class PebbleLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    PebbleLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xff1a1a1a));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xffb5b5ac));
        setColour (juce::Label::textColourId, juce::Colour (0xff1a1a1a));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xff1a1a1a));
        setColour (juce::ComboBox::backgroundColourId, juce::Colours::white);
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff8a8a82));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colours::white);
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xff1a1a1a));
    }

    /** true = big glossy white-dome cap (Density/Time/Pitch/Size/Shape);
        false = small all-black cap with a thin pointer line (the 7 trim
        pots). */
    void setWhiteCap (juce::Slider& s, bool whiteCap) { s.getProperties().set ("whiteCap", whiteCap); }

    static void drawKnurledRing (juce::Graphics& g, juce::Point<float> centre, float outerR, float innerR)
    {
        juce::ColourGradient rim (juce::Colour (0xff2a2a2a), centre.x - outerR, centre.y - outerR,
                                   juce::Colour (0xff050505), centre.x + outerR, centre.y + outerR, false);
        g.setGradientFill (rim);
        g.fillEllipse (centre.x - outerR, centre.y - outerR, outerR * 2.0f, outerR * 2.0f);

        const int notches = 40;
        for (int i = 0; i < notches; ++i)
        {
            const float a = (float) i / (float) notches * juce::MathConstants<float>::twoPi;
            juce::Line<float> notch (centre.getPointOnCircumference (innerR + (outerR - innerR) * 0.15f, a),
                                      centre.getPointOnCircumference (outerR * 0.98f, a));
            g.setColour (juce::Colour (0xff000000).withAlpha (0.5f));
            g.drawLine (notch, 0.8f);
        }
    }

    static void drawGlossyDome (juce::Graphics& g, juce::Point<float> centre, float radius, float angle, bool withPointer)
    {
        juce::DropShadow shadow (juce::Colours::black.withAlpha (0.35f), (int) (radius * 0.4f), { 0, (int) (radius * 0.18f) });
        juce::Path domePath;
        domePath.addEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        shadow.drawForPath (g, domePath);

        juce::ColourGradient gloss (juce::Colours::white,
                                     centre.x - radius * 0.5f, centre.y - radius * 0.6f,
                                     juce::Colour (0xffd2d0ca),
                                     centre.x + radius * 0.6f, centre.y + radius * 0.7f, true);
        gloss.addColour (0.35, juce::Colours::white);
        gloss.addColour (0.75, juce::Colour (0xffe8e6e0));
        g.setGradientFill (gloss);
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        auto hl = juce::Rectangle<float> (radius * 0.55f, radius * 0.4f)
                      .withCentre ({ centre.x - radius * 0.32f, centre.y - radius * 0.38f });
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillEllipse (hl);

        g.setColour (juce::Colour (0xffb8b6b0));
        g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        if (withPointer)
        {
            juce::Path pointer;
            const float pointerLength = radius * 0.72f;
            const float pointerThickness = juce::jmax (2.0f, radius * 0.1f);
            pointer.addRoundedRectangle (-pointerThickness * 0.5f, -radius * 0.88f, pointerThickness, pointerLength, pointerThickness * 0.45f);
            g.setColour (juce::Colour (0xffcac8c2));
            g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
        }
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                            float sliderPosProportional, float rotaryStartAngle,
                            float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        const bool whiteCap = slider.getProperties().contains ("whiteCap")
                                   ? (bool) slider.getProperties()["whiteCap"] : false;

        drawKnurledRing (g, centre, radius, radius * 0.7f);

        if (whiteCap)
        {
            drawGlossyDome (g, centre, radius * 0.72f, angle, true);
        }
        else
        {
            const float capRadius = radius * 0.6f;
            juce::ColourGradient cap (juce::Colour (0xff3a3a3a), centre.x - capRadius * 0.4f, centre.y - capRadius * 0.5f,
                                       juce::Colour (0xff0a0a0a), centre.x + capRadius * 0.5f, centre.y + capRadius * 0.6f, true);
            g.setGradientFill (cap);
            g.fillEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

            juce::Path pointer;
            const float pointerLength = capRadius * 0.82f;
            const float pointerThickness = juce::jmax (1.6f, radius * 0.09f);
            pointer.addRoundedRectangle (-pointerThickness * 0.5f, -capRadius * 0.9f, pointerThickness, pointerLength, pointerThickness * 0.4f);
            g.setColour (juce::Colours::white.withAlpha (0.92f));
            g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
        }
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool) override
    {
        // Round glossy dome button (FREEZE / SEED), lit when toggled on.
        auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centre = bounds.getCentre();

        juce::DropShadow shadow (juce::Colours::black.withAlpha (0.3f), (int) (radius * 0.35f), { 0, (int) (radius * 0.15f) });
        juce::Path domePath;
        domePath.addEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        shadow.drawForPath (g, domePath);

        const bool lit = button.getToggleState();
        auto baseA = lit ? juce::Colour (0xff35c9ad) : juce::Colours::white;
        auto baseB = lit ? juce::Colour (0xff1f8f7a) : juce::Colour (0xffd2d0ca);

        juce::ColourGradient gloss (juce::Colours::white.interpolatedWith (baseA, 0.15f),
                                     centre.x - radius * 0.5f, centre.y - radius * 0.6f,
                                     baseB, centre.x + radius * 0.6f, centre.y + radius * 0.7f, true);
        g.setGradientFill (gloss);
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        auto hl = juce::Rectangle<float> (radius * 0.55f, radius * 0.4f)
                      .withCentre ({ centre.x - radius * 0.3f, centre.y - radius * 0.38f });
        g.setColour (juce::Colours::white.withAlpha (lit ? 0.5f : 0.85f));
        g.fillEllipse (hl);

        g.setColour (juce::Colour (0xffb8b6b0));
        g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    }
};

} // namespace bds

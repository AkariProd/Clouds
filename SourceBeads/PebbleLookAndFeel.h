#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace bds
{

/** Real knob/button textures cropped from the hardware's own product
    photo (see SourceBeads/assets/), not hand-drawn gradients. Knobs are
    the actual photographed image rotated around its centre per the
    slider's value -- real 3D shading/highlight/knurling baked in, the
    same technique commercial plugin UIs use for photographed knobs. */
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

        knobBig_ = juce::ImageCache::getFromMemory (BinaryData::knob_big_png, BinaryData::knob_big_pngSize);
        knobSmall_ = juce::ImageCache::getFromMemory (BinaryData::knob_small_png, BinaryData::knob_small_pngSize);
        buttonRound_ = juce::ImageCache::getFromMemory (BinaryData::button_round_png, BinaryData::button_round_pngSize);
        jack_ = juce::ImageCache::getFromMemory (BinaryData::jack_png, BinaryData::jack_pngSize);
    }

    /** true = big knob photo (Density/Time/Pitch/Size/Shape); false =
        small trim-pot photo (the 7 small knobs). */
    void setWhiteCap (juce::Slider& s, bool whiteCap) { s.getProperties().set ("whiteCap", whiteCap); }

    const juce::Image& jackImage() const { return jack_; }

    static void drawSoftShadow (juce::Graphics& g, juce::Point<float> centre, float radius)
    {
        juce::DropShadow shadow (juce::Colours::black.withAlpha (0.3f), juce::jmax (2, (int) (radius * 0.12f)), { 0, juce::jmax (1, (int) (radius * 0.08f)) });
        juce::Path p;
        p.addEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        shadow.drawForPath (g, p);
    }

    static void drawRotatedImage (juce::Graphics& g, const juce::Image& img, juce::Point<float> centre,
                                   float diameter, float angle)
    {
        if (! img.isValid())
            return;

        juce::Graphics::ScopedSaveState save (g);
        g.addTransform (juce::AffineTransform::rotation (angle, centre.x, centre.y));
        g.drawImage (img, centre.x - diameter * 0.5f, centre.y - diameter * 0.5f, diameter, diameter,
                      0, 0, img.getWidth(), img.getHeight());
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                            float sliderPosProportional, float rotaryStartAngle,
                            float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
        const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto centre = bounds.getCentre();
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        const bool whiteCap = slider.getProperties().contains ("whiteCap")
                                   ? (bool) slider.getProperties()["whiteCap"] : false;

        drawRotatedImage (g, whiteCap ? knobBig_ : knobSmall_, centre, diameter, angle);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
        const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();

        g.drawImage (buttonRound_, centre.x - diameter * 0.5f, centre.y - diameter * 0.5f, diameter, diameter,
                      0, 0, buttonRound_.getWidth(), buttonRound_.getHeight());

        if (button.getToggleState())
        {
            // Tint overlay so "on" reads clearly against the photo, which
            // has no lit state of its own baked in.
            g.setColour (juce::Colour (0xff35c9ad).withAlpha (0.45f));
            g.fillEllipse (centre.x - diameter * 0.5f, centre.y - diameter * 0.5f, diameter, diameter);
        }
    }

private:
    juce::Image knobBig_, knobSmall_, buttonRound_, jack_;
};

} // namespace bds

#include "LookAndFeelP12.h"

const juce::Colour LookAndFeelP12::panelBg        { 0xff2a2724 };
const juce::Colour LookAndFeelP12::panelBgDark    { 0xff1f1c1a };
const juce::Colour LookAndFeelP12::sectionHeaderBg{ 0xff6b2f2a };
const juce::Colour LookAndFeelP12::accent         { 0xffd97a3d };
const juce::Colour LookAndFeelP12::knobCap        { 0xff3d3a36 };
const juce::Colour LookAndFeelP12::knobRing       { 0xff1a1715 };
const juce::Colour LookAndFeelP12::textColour     { 0xffe8e2d8 };

LookAndFeelP12::LookAndFeelP12()
{
    setColour(juce::ResizableWindow::backgroundColourId, panelBgDark);
    setColour(juce::Label::textColourId, textColour);
    setColour(juce::ComboBox::backgroundColourId, panelBgDark);
    setColour(juce::ComboBox::textColourId, textColour);
    setColour(juce::ComboBox::outlineColourId, knobCap);
    setColour(juce::ToggleButton::textColourId, textColour);
    setColour(juce::PopupMenu::backgroundColourId, panelBgDark);
    setColour(juce::PopupMenu::textColourId, textColour);
}

void LookAndFeelP12::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                       juce::Slider&)
{
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(2.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Track (full travel), dim
    juce::Path track;
    track.addCentredArc(centre.x, centre.y, radius * 0.92f, radius * 0.92f, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(knobCap.darker(0.6f));
    g.strokePath(track, juce::PathStrokeType(2.0f));

    // Value arc, accent colour
    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius * 0.92f, radius * 0.92f, 0.0f,
                            rotaryStartAngle, angle, true);
    g.setColour(accent);
    g.strokePath(valueArc, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Flat cap
    auto capRadius = radius * 0.72f;
    juce::Rectangle<float> capBounds(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);
    juce::ColourGradient grad(knobCap.brighter(0.15f), capBounds.getTopLeft(),
                               knobCap.darker(0.35f), capBounds.getBottomRight(), false);
    g.setGradientFill(grad);
    g.fillEllipse(capBounds);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawEllipse(capBounds, 1.0f);

    // Pointer
    juce::Path pointer;
    auto pointerLength = capRadius * 0.82f;
    auto pointerThickness = 2.4f;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength * 0.62f, 1.0f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
    g.setColour(accent.brighter(0.3f));
    g.fillPath(pointer);
}

void LookAndFeelP12::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                       bool shouldDrawButtonAsHighlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto diameter = juce::jmin(bounds.getHeight(), 18.0f);
    juce::Rectangle<float> led(bounds.getX(), bounds.getCentreY() - diameter * 0.5f, diameter, diameter);

    g.setColour(button.getToggleState() ? accent : knobCap.darker(0.4f));
    g.fillEllipse(led);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawEllipse(led, 1.0f);

    g.setColour(shouldDrawButtonAsHighlighted ? textColour.brighter() : textColour);
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawFittedText(button.getButtonText(), bounds.getX() + (int) diameter + 6, (int) bounds.getY(),
                      (int) bounds.getWidth() - (int) diameter - 6, (int) bounds.getHeight(),
                      juce::Justification::centredLeft, 1);
}

juce::Font LookAndFeelP12::getLabelFont(juce::Label&)
{
    return juce::Font(juce::FontOptions(12.5f));
}

juce::Font LookAndFeelP12::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions(12.5f));
}

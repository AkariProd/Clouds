#include "PluginEditor.h"

namespace
{
const juce::Colour kCyan   (0xff29c7d6);
const juce::Colour kMagenta(0xffd6316f);
const juce::Colour kPanel  (0xfff4f3ef);
const juce::Colour kInk    (0xff1a1a1a);
}

ContourEditor::ContourEditor (ContourProcessor& p)
    : juce::AudioProcessorEditor (&p), proc_ (p)
{
    setLookAndFeel (&lnf_);

    makeKnob (mapX_, grd::pid::mapX, "MAP X");
    makeKnob (mapY_, grd::pid::mapY, "MAP Y");
    makeKnob (chaos_, grd::pid::chaos, "CHAOS");

    makeKnob (bdFill_, grd::pid::bdDensity, "BD", kCyan);
    makeKnob (sdFill_, grd::pid::sdDensity, "SD", kCyan);
    makeKnob (hhFill_, grd::pid::hhDensity, "HH", kCyan);

    makeKnob (euclidBd_, grd::pid::euclidBd, "BD LEN");
    makeKnob (euclidSd_, grd::pid::euclidSd, "SD LEN");
    makeKnob (euclidHh_, grd::pid::euclidHh, "HH LEN");

    addAndMakeVisible (swingButton_);
    swingAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, grd::pid::swing, swingButton_);

    resetButton_.onClick = [this] { proc_.requestReset(); };
    addAndMakeVisible (resetButton_);

    modeBox_.addItemList ({ "Drums (topographic)", "Euclidean" }, 1);
    addAndMakeVisible (modeBox_);
    modeLabel_.setJustificationType (juce::Justification::centred);
    modeLabel_.setColour (juce::Label::textColourId, juce::Colour (0xff777770));
    modeLabel_.setFont (juce::Font (juce::FontOptions (10.0f)).boldened());
    addAndMakeVisible (modeLabel_);
    modeAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc_.apvts, grd::pid::mode, modeBox_);

    setSize (460, 580);
    startTimerHz (30);
}

ContourEditor::~ContourEditor()
{
    setLookAndFeel (nullptr);
}

void ContourEditor::makeKnob (Knob& k, const juce::String& paramId, const juce::String& text, juce::Colour accent)
{
    addAndMakeVisible (k.slider);
    if (accent != juce::Colour())
        lnf_.setKnobAccent (k.slider, accent);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::Font (juce::FontOptions (11.0f)).boldened());
    k.label.setColour (juce::Label::textColourId, kInk);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc_.apvts, paramId, k.slider);
}

void ContourEditor::timerCallback()
{
    repaint (ledArea_[0].getUnion (ledArea_[1]).getUnion (ledArea_[2]).expanded (4));
}

void ContourEditor::paint (juce::Graphics& g)
{
    g.fillAll (kPanel);

    g.setColour (juce::Colour (0xffb5b5ac));
    for (auto pt : { juce::Point<int> (12, 12), juce::Point<int> (getWidth() - 12, 12),
                      juce::Point<int> (12, getHeight() - 12), juce::Point<int> (getWidth() - 12, getHeight() - 12) })
        g.fillEllipse ((float) pt.x - 3.0f, (float) pt.y - 3.0f, 6.0f, 6.0f);

    // Faint paisley/mandala decoration behind the right-hand knob column,
    // echoing the panel's own screened pattern.
    auto decoArea = getLocalBounds().reduced (16, 10).withTrimmedTop (50).withHeight (260);
    g.setColour (juce::Colour (0xffe6e4de));
    auto c = juce::Point<float> ((float) decoArea.getRight() - 90.0f, (float) decoArea.getCentreY());
    for (int r = 10; r <= 110; r += 14)
        g.drawEllipse (c.x - (float) r, c.y - (float) r, (float) r * 2.0f, (float) r * 2.0f, 0.6f);

    auto header = getLocalBounds().reduced (16, 10).removeFromTop (34);
    g.setColour (kInk);
    g.setFont (juce::Font (juce::FontOptions (24.0f)).boldened());
    g.drawText ("Contour", header, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.setColour (juce::Colour (0xff555550));
    g.drawMultiLineText ("topographic\ndrum sequencer", header.getRight() - 110, header.getY() + 12, 110);

    g.setFont (juce::Font (juce::FontOptions (10.0f)).boldened());
    g.setColour (kMagenta);
    auto swingLabelArea = swingButton_.getBounds().withY (swingButton_.getBottom() + 2).withHeight (14);
    g.drawText ("SWING", swingLabelArea, juce::Justification::centred);

    g.setColour (juce::Colour (0xff777770));
    auto resetLabelArea = resetButton_.getBounds().withY (resetButton_.getBottom() + 2).withHeight (14);
    g.drawText ("RESET", resetLabelArea, juce::Justification::centred);

    g.setColour (kCyan);
    auto fillLabelArea = bdFill_.slider.getBounds().getUnion (hhFill_.slider.getBounds())
                              .withY (bdFill_.slider.getBounds().getY() - 16).withHeight (12);
    g.drawText ("FILL", fillLabelArea, juce::Justification::centred);

    // Per-part trigger LEDs, lit live from the engine's current state.
    static const juce::Colour ledColours[3] = { kMagenta, kCyan, juce::Colour (0xfff2a93c) };
    for (int i = 0; i < 3; ++i)
    {
        auto lit = proc_.partActive (i);
        g.setColour (lit ? ledColours[i] : juce::Colour (0xffd8d6d0));
        g.fillEllipse (ledArea_[i].toFloat());
        g.setColour (juce::Colour (0xff8a8a82));
        g.drawEllipse (ledArea_[i].toFloat(), 1.0f);
    }

    auto badge = getLocalBounds().removeFromBottom (24).reduced (16, 2);
    g.setColour (juce::Colour (0xff777770));
    g.setFont (juce::Font (juce::FontOptions (10.0f)).boldened());
    g.drawText ("SOULMADE", badge, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText ("after Mutable Instruments' Grids (GPL-3.0) + original drum synthesis", badge, juce::Justification::centredRight);
}

void ContourEditor::resized()
{
    auto area = getLocalBounds().reduced (16, 10);
    area.removeFromTop (38);
    area.removeFromBottom (26);

    // Top row: SWING (big round button, CLOCK knob's slot) + RESET/TAP.
    auto topRow = area.removeFromTop (90);
    swingButton_.setBounds (topRow.removeFromLeft (70).withSizeKeepingCentre (64, 64));
    topRow.removeFromLeft (10);
    resetButton_.setBounds (topRow.removeFromLeft (56).withSizeKeepingCentre (48, 48));

    area.removeFromTop (18);

    // Left column: MAP X / MAP Y / CHAOS stacked; right column: 3 small
    // FILL knobs with LEDs, matching the photo's two-column layout.
    auto body = area.removeFromTop (280);
    auto leftCol = body.removeFromLeft (body.getWidth() * 3 / 5);
    const int bigSize = 78;
    auto mapXCell = leftCol.removeFromTop (leftCol.getHeight() / 3);
    mapX_.slider.setBounds (mapXCell.withSizeKeepingCentre (bigSize, bigSize));
    mapX_.label.setBounds (mapXCell.withSizeKeepingCentre (bigSize, 16).withY (mapXCell.getBottom() - 18));
    auto mapYCell = leftCol.removeFromTop (leftCol.getHeight() / 2);
    mapY_.slider.setBounds (mapYCell.withSizeKeepingCentre (bigSize, bigSize));
    mapY_.label.setBounds (mapYCell.withSizeKeepingCentre (bigSize, 16).withY (mapYCell.getBottom() - 18));
    auto chaosCell = leftCol;
    chaos_.slider.setBounds (chaosCell.withSizeKeepingCentre (bigSize, bigSize));
    chaos_.label.setBounds (chaosCell.withSizeKeepingCentre (bigSize, 16).withY (chaosCell.getBottom() - 18));

    body.removeFromTop (20); // room for the FILL label drawn in paint()
    const int smallSize = 56;
    auto fillCell1 = body.removeFromTop (body.getHeight() / 3);
    bdFill_.slider.setBounds (fillCell1.withSizeKeepingCentre (smallSize, smallSize).withX (fillCell1.getX() + 10));
    bdFill_.label.setBounds (bdFill_.slider.getBounds().withY (bdFill_.slider.getBounds().getBottom()).withHeight (14));
    ledArea_[0] = juce::Rectangle<int> (10, 10).withCentre (
        { fillCell1.getRight() - 16, fillCell1.getCentreY() });

    auto fillCell2 = body.removeFromTop (body.getHeight() / 2);
    sdFill_.slider.setBounds (fillCell2.withSizeKeepingCentre (smallSize, smallSize).withX (fillCell2.getX() + 10));
    sdFill_.label.setBounds (sdFill_.slider.getBounds().withY (sdFill_.slider.getBounds().getBottom()).withHeight (14));
    ledArea_[1] = juce::Rectangle<int> (10, 10).withCentre (
        { fillCell2.getRight() - 16, fillCell2.getCentreY() });

    auto fillCell3 = body;
    hhFill_.slider.setBounds (fillCell3.withSizeKeepingCentre (smallSize, smallSize).withX (fillCell3.getX() + 10));
    hhFill_.label.setBounds (hhFill_.slider.getBounds().withY (hhFill_.slider.getBounds().getBottom()).withHeight (14));
    ledArea_[2] = juce::Rectangle<int> (10, 10).withCentre (
        { fillCell3.getRight() - 16, fillCell3.getCentreY() });

    area.removeFromTop (12);

    // Euclidean-length knobs: small row, only meaningful in Euclidean mode
    // (not on the real hardware panel -- a bonus control from the
    // firmware's second output algorithm).
    auto euclidRow = area.removeFromTop (64);
    Knob* euclidKnobs[] = { &euclidBd_, &euclidSd_, &euclidHh_ };
    const int ew = euclidRow.getWidth() / 3;
    for (auto* k : euclidKnobs)
    {
        auto cell = euclidRow.removeFromLeft (ew);
        k->slider.setBounds (cell.withTrimmedBottom (14));
        k->label.setBounds (cell.removeFromBottom (14));
    }

    area.removeFromTop (10);

    auto modeRow = area.removeFromTop (24);
    modeLabel_.setBounds (modeRow.removeFromLeft (120));
    area.removeFromTop (0);
    auto modeControlRow = area.removeFromTop (26);
    modeBox_.setBounds (modeControlRow.removeFromLeft (160));

    area.removeFromTop (10);
}

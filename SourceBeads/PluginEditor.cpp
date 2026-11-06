#include "PluginEditor.h"
#include "Parameters.h"

// ---------------------------------------------------------------------------
// PillSwitch
// ---------------------------------------------------------------------------

PillSwitch::PillSwitch (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                         juce::Colour colour, bds::PillIcon icon)
    : colour_ (colour), icon_ (icon),
      attachment_ (*apvts.getParameter (paramId),
                    [this] (float v) { value_ = v; repaint(); },
                    apvts.undoManager)
{
    value_ = apvts.getParameter (paramId)->getValue();
    setRepaintsOnMouseActivity (true);
}

void PillSwitch::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasDraggedSinceMouseDown())
    {
        const float newValue = value_ > 0.5f ? 0.0f : 1.0f;
        attachment_.setValueAsCompleteGesture (newValue);
    }
}

void PillSwitch::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const bool on = value_ > 0.5f;

    juce::DropShadow shadow (juce::Colours::black.withAlpha (0.25f), 4, { 0, 2 });
    juce::Path pillPath;
    pillPath.addRoundedRectangle (bounds, bounds.getWidth() * 0.5f);
    shadow.drawForPath (g, pillPath);

    juce::ColourGradient grad (colour_.brighter (0.15f), bounds.getX(), bounds.getY(),
                                colour_.darker (0.25f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillPath (pillPath);
    g.setColour (juce::Colours::black.withAlpha (0.15f));
    g.strokePath (pillPath, juce::PathStrokeType (1.0f));

    auto iconArea = bounds.removeFromTop (bounds.getHeight() * 0.55f).reduced (bounds.getWidth() * 0.26f);
    g.setColour (juce::Colours::white.withAlpha (0.95f));

    if (icon_ == bds::PillIcon::loop)
    {
        juce::Path arc;
        arc.addCentredArc (iconArea.getCentreX(), iconArea.getCentreY(), iconArea.getWidth() * 0.45f,
                            iconArea.getHeight() * 0.45f, 0.0f, 0.6f, juce::MathConstants<float>::twoPi - 0.3f, true);
        g.strokePath (arc, juce::PathStrokeType (1.8f));
        auto tip = juce::Point<float> (iconArea.getCentreX(), iconArea.getCentreY()).getPointOnCircumference (
            iconArea.getWidth() * 0.45f, 0.6f);
        juce::Path arrow;
        arrow.addTriangle (tip.x - 3.0f, tip.y - 4.0f, tip.x + 4.0f, tip.y, tip.x - 3.0f, tip.y + 4.0f);
        g.fillPath (arrow);
    }
    else if (icon_ == bds::PillIcon::cloud)
    {
        auto c = iconArea.getCentre();
        const float r = iconArea.getWidth() * 0.22f;
        g.fillEllipse (c.x - r * 1.5f, c.y - r * 0.2f, r * 1.6f, r * 1.6f);
        g.fillEllipse (c.x - r * 0.3f, c.y - r * 1.0f, r * 1.8f, r * 1.8f);
        g.fillEllipse (c.x + r * 0.6f, c.y - r * 0.2f, r * 1.5f, r * 1.5f);
        g.fillRoundedRectangle (c.x - r * 1.4f, c.y + r * 0.5f, r * 3.0f, r * 1.0f, r * 0.5f);
    }
    else // cube
    {
        auto c = iconArea.getCentre();
        const float s = iconArea.getWidth() * 0.4f;
        juce::Path cube;
        cube.startNewSubPath (c.x, c.y - s);
        cube.lineTo (c.x + s * 0.9f, c.y - s * 0.5f);
        cube.lineTo (c.x + s * 0.9f, c.y + s * 0.5f);
        cube.lineTo (c.x, c.y + s);
        cube.lineTo (c.x - s * 0.9f, c.y + s * 0.5f);
        cube.lineTo (c.x - s * 0.9f, c.y - s * 0.5f);
        cube.closeSubPath();
        cube.startNewSubPath (c.x, c.y - s);
        cube.lineTo (c.x, c.y);
        cube.lineTo (c.x + s * 0.9f, c.y - s * 0.5f);
        cube.startNewSubPath (c.x, c.y);
        cube.lineTo (c.x - s * 0.9f, c.y - s * 0.5f);
        cube.startNewSubPath (c.x, c.y);
        cube.lineTo (c.x, c.y + s);
        g.strokePath (cube, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    auto ledArea = getLocalBounds().toFloat();
    const float ledD = ledArea.getWidth() * 0.5f;
    auto ledCentre = juce::Point<float> (ledArea.getCentreX(), ledArea.getBottom() - ledArea.getWidth() * 0.62f);
    g.setColour (on ? juce::Colour (0xfff5e36a) : juce::Colour (0xfff5e36a).withAlpha (0.35f));
    g.fillEllipse (ledCentre.x - ledD * 0.5f, ledCentre.y - ledD * 0.5f, ledD, ledD);
    if (on)
    {
        g.setColour (juce::Colour (0xfff5e36a).withAlpha (0.4f));
        g.fillEllipse (ledCentre.x - ledD * 0.75f, ledCentre.y - ledD * 0.75f, ledD * 1.5f, ledD * 1.5f);
    }
}

// ---------------------------------------------------------------------------
// PebbleEditor
// ---------------------------------------------------------------------------

namespace
{
const juce::Colour kCyan   (0xff2bb8c4);
const juce::Colour kOrange (0xfff2a93c);
const juce::Colour kMagenta(0xffd63a8f);
const juce::Colour kPanel  (0xfff6f5f1);
const juce::Colour kInk    (0xff1a1a1a);
}

PebbleEditor::PebbleEditor (PebbleProcessor& p)
    : juce::AudioProcessorEditor (&p), proc_ (p)
{
    setLookAndFeel (&lnf_);

    makeKnob (density_, bds::pid::density, "DENSITY", true);
    makeKnob (time_,    bds::pid::time,    "TIME", true);
    makeKnob (pitch_,   bds::pid::pitch,   "PITCH", true);
    makeKnob (size_,    bds::pid::size,    "SIZE", true);
    makeKnob (shape_,   bds::pid::shape,   "SHAPE", true);

    makeKnob (feedback_,    bds::pid::feedback,    "", false);
    makeKnob (dryWet_,      bds::pid::dryWet,      "", false);
    makeKnob (reverb_,      bds::pid::reverb,      "", false);
    makeKnob (timeAmount_,  bds::pid::timeAmount,  "", false);
    makeKnob (sizeAmount_,  bds::pid::sizeAmount,  "", false);
    makeKnob (shapeAmount_, bds::pid::shapeAmount, "", false);
    makeKnob (pitchAmount_, bds::pid::pitchAmount, "", false);

    for (auto* b : { &freezeBtn_, &seedBtn_ })
        addAndMakeVisible (b);
    for (auto* l : { &freezeLabel_, &seedLabel_ })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setFont (juce::Font (juce::FontOptions (13.0f)).boldened());
        l->setColour (juce::Label::textColourId, kInk);
        addAndMakeVisible (l);
    }
    freezeAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::freeze, freezeBtn_);
    seedAtt_   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::seed, seedBtn_);

    seedLockPill_   = std::make_unique<PillSwitch> (proc_.apvts, bds::pid::seedLock,   kCyan,    bds::PillIcon::loop);
    monoGrainPill_  = std::make_unique<PillSwitch> (proc_.apvts, bds::pid::monoGrain,  kOrange,  bds::PillIcon::cloud);
    outputGatePill_ = std::make_unique<PillSwitch> (proc_.apvts, bds::pid::outputGate, kMagenta, bds::PillIcon::cube);
    for (auto* pill : { seedLockPill_.get(), monoGrainPill_.get(), outputGatePill_.get() })
        addAndMakeVisible (pill);

    mediaBox_.addItemList ({ "Dirty Tape", "Clean Tape", "Vintage Digital", "Digital" }, 1);
    mediaBox_.setColour (juce::ComboBox::backgroundColourId, juce::Colours::white);
    addAndMakeVisible (mediaBox_);
    mediaLabel_.setJustificationType (juce::Justification::centredLeft);
    mediaLabel_.setFont (juce::Font (juce::FontOptions (10.0f)).boldened());
    mediaLabel_.setColour (juce::Label::textColourId, juce::Colour (0xff8a8880));
    addAndMakeVisible (mediaLabel_);
    mediaAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc_.apvts, bds::pid::media, mediaBox_);

    setSize (490, 1180);
}

PebbleEditor::~PebbleEditor()
{
    setLookAndFeel (nullptr);
}

void PebbleEditor::makeKnob (Knob& k, const juce::String& paramId, const juce::String& text, bool whiteCap)
{
    addAndMakeVisible (k.slider);
    lnf_.setWhiteCap (k.slider, whiteCap);

    if (text.isNotEmpty())
    {
        k.label.setText (text, juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (juce::Font (juce::FontOptions (13.0f)).boldened());
        k.label.setColour (juce::Label::textColourId, kInk);
        addAndMakeVisible (k.label);
    }

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc_.apvts, paramId, k.slider);
}

void PebbleEditor::drawJack (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    juce::DropShadow shadow (juce::Colours::black.withAlpha (0.25f), (int) (radius * 0.5f), { 0, (int) (radius * 0.2f) });
    juce::Path p;
    p.addEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    shadow.drawForPath (g, p);

    juce::ColourGradient chrome (juce::Colour (0xffe8e4d8), centre.x - radius, centre.y - radius,
                                  juce::Colour (0xff9a9488), centre.x + radius, centre.y + radius, false);
    g.setGradientFill (chrome);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    for (int i = 0; i < 20; ++i)
    {
        const float a = (float) i / 20.0f * juce::MathConstants<float>::twoPi;
        g.setColour (juce::Colour (0xff5a564c).withAlpha (0.4f));
        g.drawLine (juce::Line<float> (centre.getPointOnCircumference (radius * 0.82f, a),
                                        centre.getPointOnCircumference (radius * 0.98f, a)), 0.6f);
    }

    const float innerR = radius * 0.5f;
    g.setColour (juce::Colour (0xff0a0a0a));
    g.fillEllipse (centre.x - innerR, centre.y - innerR, innerR * 2.0f, innerR * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.fillEllipse (centre.x - radius * 0.6f, centre.y - radius * 0.7f, radius * 0.3f, radius * 0.18f);
}

void PebbleEditor::drawDotted (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b)
{
    juce::Path line;
    line.startNewSubPath (a);
    line.lineTo (b);
    juce::Path dashed;
    const float dashLengths[] = { 2.5f, 2.5f };
    juce::PathStrokeType (1.0f).createDashedStroke (dashed, line, dashLengths, 2);
    g.setColour (juce::Colour (0xff8a8880));
    g.fillPath (dashed);
}

void PebbleEditor::paint (juce::Graphics& g)
{
    g.fillAll (kPanel);

    g.setColour (juce::Colour (0xffb5b5ac));
    for (auto pt : { juce::Point<int> (18, 18), juce::Point<int> (getWidth() - 18, 18),
                      juce::Point<int> (18, getHeight() - 18), juce::Point<int> (getWidth() - 18, getHeight() - 18) })
        g.fillEllipse ((float) pt.x - 4.0f, (float) pt.y - 4.0f, 8.0f, 8.0f);

    auto header = getLocalBounds().reduced (28, 16).removeFromTop (46);
    auto iconBox = header.removeFromLeft (34).toFloat();
    g.setColour (kInk);
    g.drawRoundedRectangle (iconBox, 4.0f, 1.6f);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            g.fillEllipse (iconBox.getX() + 5.0f + (float) c * 8.0f, iconBox.getY() + 5.0f + (float) r * 8.0f, 3.0f, 3.0f);
    header.removeFromLeft (10);
    g.setFont (juce::Font (juce::FontOptions (30.0f)).boldened());
    g.drawText ("Pebble", header, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.setColour (juce::Colour (0xff6a6860));
    g.drawMultiLineText ("texture\nsynthesizer", header.getRight() - 110, header.getY() + 14, 110,
                          juce::Justification::right);

    auto divider = getLocalBounds().reduced (28, 0).withY (74).withHeight (10);
    g.setColour (juce::Colour (0xffc8c6be));
    for (int x = divider.getX(); x < divider.getRight(); x += 10)
    {
        juce::Path chevron;
        chevron.startNewSubPath ((float) x, (float) divider.getBottom());
        chevron.lineTo ((float) x + 5.0f, (float) divider.getY());
        chevron.lineTo ((float) x + 10.0f, (float) divider.getBottom());
        g.strokePath (chevron, juce::PathStrokeType (1.0f));
    }

    auto densityLabelBounds = density_.label.getBounds();
    for (int side = 0; side < 2; ++side)
    {
        const int baseX = side == 0 ? densityLabelBounds.getX() - 46 : densityLabelBounds.getRight() + 14;
        for (int i = 0; i < 5; ++i)
        {
            const float h = (i == 2) ? 10.0f : 7.0f;
            g.setColour (juce::Colour (0xffb0ada4));
            g.fillRect ((float) baseX + (float) i * 6.0f, (float) densityLabelBounds.getCentreY() - h * 0.5f, 1.4f, h);
        }
    }

    auto legendCentre = juce::Point<float> ((float) getWidth() / 2.0f, (float) densityLabelBounds.getBottom() + 26.0f);
    g.setColour (juce::Colour (0xffcac8c0));
    g.fillEllipse (legendCentre.x - 36.0f, legendCentre.y - 6.0f, 12.0f, 12.0f);
    g.setColour (juce::Colour (0xff2a2a2a));
    g.fillEllipse (legendCentre.x - 14.0f, legendCentre.y - 6.0f, 12.0f, 12.0f);
    float barX = legendCentre.x + 10.0f;
    const juce::Colour barColours[3] = { kCyan, kOrange, kMagenta };
    const float barHeights[3] = { 6.0f, 10.0f, 14.0f };
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (barColours[(size_t) i]);
        g.fillRect (barX, legendCentre.y + 7.0f - barHeights[(size_t) i], 4.0f, barHeights[(size_t) i]);
        barX += 6.0f;
    }

    drawDotted (g, timeAmount_.slider.getBounds().getCentre().toFloat().withY ((float) timeAmount_.slider.getBounds().getY()),
                knobCentres_[0]);
    drawDotted (g, sizeAmount_.slider.getBounds().getCentre().toFloat().withY ((float) sizeAmount_.slider.getBounds().getY()),
                knobCentres_[1]);
    drawDotted (g, shapeAmount_.slider.getBounds().getCentre().toFloat().withY ((float) shapeAmount_.slider.getBounds().getY()),
                knobCentres_[2]);
    drawDotted (g, pitchAmount_.slider.getBounds().getCentre().toFloat().withY ((float) pitchAmount_.slider.getBounds().getY()),
                knobCentres_[3]);

    for (auto& c : cvJackCentres_)
        drawJack (g, c, 15.0f);
    g.setColour (juce::Colour (0xff8a8880));
    g.setFont (juce::Font (juce::FontOptions (9.0f)).boldened());
    g.drawText ("CV IN", (int) cvJackCentres_[0].x - 40, (int) cvJackCentres_[0].y - 34, 80, 12, juce::Justification::centred);

    for (int i = 0; i < 4; ++i)
        drawJack (g, ioJackCentres_[(size_t) i], 15.0f);

    auto lrBox = juce::Rectangle<float> (ioJackCentres_[4].x - 30.0f, ioJackCentres_[4].y - 34.0f,
                                          ioJackCentres_[5].x - ioJackCentres_[4].x + 60.0f, 80.0f);
    g.setColour (juce::Colour (0xff6b7078));
    g.fillRoundedRectangle (lrBox, 6.0f);
    drawJack (g, ioJackCentres_[4], 15.0f);
    drawJack (g, ioJackCentres_[5], 15.0f);
    drawDotted (g, ioJackCentres_[4], ioJackCentres_[5]);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (11.0f)).boldened());
    g.drawText ("L", (int) ioJackCentres_[4].x - 10, (int) lrBox.getY() + 2, 20, 14, juce::Justification::centred);
    g.drawText ("R", (int) ioJackCentres_[5].x - 10, (int) lrBox.getY() + 2, 20, 14, juce::Justification::centred);

    static const char* ioLabels[4] = { "IN L", "IN R", "FREEZE", "SEED" };
    g.setColour (kInk);
    g.setFont (juce::Font (juce::FontOptions (11.0f)).boldened());
    for (int i = 0; i < 4; ++i)
        g.drawText (ioLabels[i], (int) ioJackCentres_[(size_t) i].x - 34, (int) ioJackCentres_[0].y - 34, 68, 14,
                    juce::Justification::centred);

    drawDotted (g, ioJackCentres_[0], ioJackCentres_[1]);
    g.setColour (juce::Colour (0xfff5e36a));
    auto ledMid = juce::Point<float> ((ioJackCentres_[0].x + ioJackCentres_[1].x) * 0.5f,
                                       (ioJackCentres_[0].y + ioJackCentres_[1].y) * 0.5f);
    g.fillEllipse (ledMid.x - 4.0f, ledMid.y - 20.0f, 8.0f, 8.0f);

    auto footerDivider = getLocalBounds().reduced (28, 0).withY (getHeight() - 70).withHeight (1);
    g.setColour (juce::Colour (0xffc8c6be));
    g.fillRect (footerDivider);

    auto badge = getLocalBounds().reduced (28, 0).withY (getHeight() - 52).withHeight (30);
    auto badgeIcon = badge.removeFromLeft (26).toFloat();
    g.setColour (juce::Colour (0xff444138));
    g.fillRoundedRectangle (badgeIcon.reduced (2.0f), 6.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (badgeIcon.getCentreX() - 5.0f, badgeIcon.getCentreY() - 5.0f, 10.0f, 10.0f);
    badge.removeFromLeft (8);
    g.setColour (kInk);
    g.setFont (juce::Font (juce::FontOptions (17.0f)).boldened());
    g.drawText ("SoulMade", badge, juce::Justification::centredLeft);

    auto subBadge = getLocalBounds().reduced (28, 0).withY (getHeight() - 24).withHeight (16);
    g.setFont (juce::Font (juce::FontOptions (9.5f)));
    g.setColour (juce::Colour (0xff8a8880));
    g.drawText ("DSP after Mutable Instruments' Beads (MIT)", subBadge, juce::Justification::centredLeft);
}

void PebbleEditor::resized()
{
    auto area = getLocalBounds().reduced (28, 16);
    area.removeFromTop (46);
    area.removeFromTop (22);

    auto topRow = area.removeFromTop (150);
    freezeBtn_.setBounds (topRow.removeFromLeft (72).withSizeKeepingCentre (56, 56).withY (topRow.getY() + 14));
    freezeLabel_.setBounds (freezeBtn_.getBounds().withY (freezeBtn_.getBottom() + 4).withHeight (16).expanded (20, 0));
    seedBtn_.setBounds (topRow.removeFromRight (72).withSizeKeepingCentre (56, 56).withY (topRow.getY() + 14));
    seedLabel_.setBounds (seedBtn_.getBounds().withY (seedBtn_.getBottom() + 4).withHeight (16).expanded (20, 0));
    density_.slider.setBounds (topRow.withSizeKeepingCentre (128, 128));
    density_.label.setBounds (topRow.withSizeKeepingCentre (160, 18).withY (topRow.getBottom() - 20));

    area.removeFromTop (4);
    area.removeFromTop (54); // legend row drawn in paint()

    auto bigRow = area.removeFromTop (150);
    auto timeCell = bigRow.removeFromLeft (bigRow.getWidth() / 2);
    time_.slider.setBounds (timeCell.withSizeKeepingCentre (118, 118));
    time_.label.setBounds (timeCell.withSizeKeepingCentre (140, 18).withY (timeCell.getBottom() - 20));
    pitch_.slider.setBounds (bigRow.withSizeKeepingCentre (118, 118));
    pitch_.label.setBounds (bigRow.withSizeKeepingCentre (140, 18).withY (bigRow.getBottom() - 20));

    area.removeFromTop (16);

    auto midRow = area.removeFromTop (190);
    auto leftCol = midRow.removeFromLeft (70);
    auto rightCol = midRow.removeFromRight (70);

    timeAmount_.slider.setBounds (leftCol.removeFromTop (54).withSizeKeepingCentre (46, 46));
    leftCol.removeFromTop (4);
    seedLockPill_->setBounds (leftCol.removeFromTop (110).withSizeKeepingCentre (52, 110));

    pitchAmount_.slider.setBounds (rightCol.removeFromTop (54).withSizeKeepingCentre (46, 46));
    rightCol.removeFromTop (4);
    monoGrainPill_->setBounds (rightCol.removeFromTop (110).withSizeKeepingCentre (52, 110));

    auto sizeShapeRow = midRow.removeFromTop (118);
    auto sizeCell = sizeShapeRow.removeFromLeft (sizeShapeRow.getWidth() / 2);
    size_.slider.setBounds (sizeCell.withSizeKeepingCentre (96, 96));
    size_.label.setBounds (sizeCell.withSizeKeepingCentre (120, 18).withY (sizeCell.getBottom() - 20));
    shape_.slider.setBounds (sizeShapeRow.withSizeKeepingCentre (96, 96));
    shape_.label.setBounds (sizeShapeRow.withSizeKeepingCentre (120, 18).withY (sizeShapeRow.getBottom() - 20));

    knobCentres_[0] = time_.slider.getBounds().getCentre().toFloat();
    knobCentres_[1] = size_.slider.getBounds().getCentre().toFloat();
    knobCentres_[2] = shape_.slider.getBounds().getCentre().toFloat();
    knobCentres_[3] = pitch_.slider.getBounds().getCentre().toFloat();

    area.removeFromTop (14);

    auto trimRow = area.removeFromTop (60);
    Knob* trims[] = { &feedback_, &dryWet_, &reverb_, &timeAmount_, &sizeAmount_, &shapeAmount_, &pitchAmount_ };
    const int tw = trimRow.getWidth() / 7;
    for (auto* k : trims)
    {
        auto cell = trimRow.removeFromLeft (tw);
        k->slider.setBounds (cell.withSizeKeepingCentre (44, 44));
    }

    area.removeFromTop (30);

    auto cvRow = area.removeFromTop (60);
    {
        const int w = cvRow.getWidth() / 6;
        for (int i = 0; i < 6; ++i)
            cvJackCentres_[(size_t) i] = juce::Point<float> ((float) (cvRow.getX() + w * i + w / 2), (float) cvRow.getCentreY());
    }

    area.removeFromTop (34);

    auto ioRow = area.removeFromTop (60);
    {
        const int w = ioRow.getWidth() / 6;
        for (int i = 0; i < 6; ++i)
            ioJackCentres_[(size_t) i] = juce::Point<float> ((float) (ioRow.getX() + w * i + w / 2), (float) ioRow.getCentreY());
    }

    area.removeFromTop (14);

    auto mediaRow = area.removeFromTop (28);
    mediaLabel_.setBounds (mediaRow.removeFromLeft (50));
    mediaRow.removeFromLeft (4);
    mediaBox_.setBounds (mediaRow.removeFromLeft (160));
}

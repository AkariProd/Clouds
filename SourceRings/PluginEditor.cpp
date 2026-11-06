#include "PluginEditor.h"

namespace
{
const juce::Colour kCyan   (0xff29c7d6);
const juce::Colour kMagenta(0xffd63a8f);
const juce::Colour kAmber  (0xfff2a93c);
const juce::Colour kPanel  (0xfff4f3ef);   // light cream, matches the real panel
const juce::Colour kInk    (0xff1a1a1a);
}

ChimeEditor::ChimeEditor (ChimeProcessor& p)
    : juce::AudioProcessorEditor (&p), proc_ (p)
{
    setLookAndFeel (&lnf_);

    makeKnob (frequency_, rng::pid::frequency, "FREQUENCY", kCyan);
    makeKnob (structure_, rng::pid::structure, "STRUCTURE", kMagenta);
    makeKnob (brightness_, rng::pid::brightness, "BRIGHTNESS", kAmber);
    makeKnob (damping_, rng::pid::damping, "DAMPING", kMagenta);
    makeKnob (position_, rng::pid::position, "POSITION", kAmber);

    makeKnob (frequencyAmount_, rng::pid::frequencyAmount, "FREQ", kCyan);
    makeKnob (structureAmount_, rng::pid::structureAmount, "STRUCT", kMagenta);
    makeKnob (brightnessAmount_, rng::pid::brightnessAmount, "BRIGHT", kAmber);
    makeKnob (dampingAmount_, rng::pid::dampingAmount, "DAMP", kMagenta);
    makeKnob (positionAmount_, rng::pid::positionAmount, "POS", kAmber);

    modelBox_.addItemList ({ "Modal Resonator", "Sympathetic Strings", "Modulated/Inharmonic String",
                              "FM Voice", "Sympathetic Strings (Quantized)", "String + Reverb" }, 1);
    strumBox_.addItemList ({ "External Trigger", "Note Change", "Audio Onset" }, 1);

    for (auto* c : { &modelBox_, &strumBox_ })
        addAndMakeVisible (c);
    for (auto* l : { &modelLabel_, &strumLabel_, &polyLabel_ })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::textColourId, juce::Colour (0xff777770));
        l->setFont (juce::Font (juce::FontOptions (10.0f)).boldened());
        addAndMakeVisible (l);
    }
    addAndMakeVisible (polySlider_);

    modelAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc_.apvts, rng::pid::model, modelBox_);
    strumAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc_.apvts, rng::pid::strumMode, strumBox_);
    polyAtt_  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc_.apvts, rng::pid::polyphony, polySlider_);

    setSize (460, 620);
}

ChimeEditor::~ChimeEditor()
{
    setLookAndFeel (nullptr);
}

void ChimeEditor::makeKnob (Knob& k, const juce::String& paramId, const juce::String& text, juce::Colour accent)
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

void ChimeEditor::paint (juce::Graphics& g)
{
    g.fillAll (kPanel);

    // Four corner mounting-screw dots, like the real faceplate.
    g.setColour (juce::Colour (0xffb5b5ac));
    for (auto pt : { juce::Point<int> (12, 12), juce::Point<int> (getWidth() - 12, 12),
                      juce::Point<int> (12, getHeight() - 12), juce::Point<int> (getWidth() - 12, getHeight() - 12) })
        g.fillEllipse ((float) pt.x - 3.0f, (float) pt.y - 3.0f, 6.0f, 6.0f);

    // Faint radial/mandala pattern behind the big knobs, echoing the
    // panel's own screened decoration.
    auto decoArea = getLocalBounds().reduced (16, 10).withTrimmedTop (60).withHeight (150);
    g.setColour (juce::Colour (0xffe6e4de));
    for (auto cx : { decoArea.getX() + decoArea.getWidth() / 4, decoArea.getRight() - decoArea.getWidth() / 4 })
    {
        auto c = juce::Point<float> ((float) cx, (float) decoArea.getCentreY());
        for (int r = 10; r <= 64; r += 10)
            g.drawEllipse (c.x - (float) r, c.y - (float) r, (float) r * 2.0f, (float) r * 2.0f, 0.6f);
    }

    // Dotted connector lines: Frequency->Brightness, Structure->Position,
    // and both big knobs crossing diagonally through Damping in the
    // middle — matching the photo's wiring-diagram motif.
    auto dotted = [&] (juce::Point<float> a, juce::Point<float> b)
    {
        if (a.isOrigin() || b.isOrigin())
            return;
        juce::Path line;
        line.startNewSubPath (a);
        line.lineTo (b);
        juce::Path dashed;
        const float dashLengths[] = { 3.0f, 3.0f };
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, line, dashLengths, 2);
        g.setColour (juce::Colour (0xffb5b5ac));
        g.fillPath (dashed);
    };

    dotted (freqCentre_, brightCentre_);
    dotted (structCentre_, posCentre_);
    dotted (freqCentre_, dampCentre_);
    dotted (structCentre_, dampCentre_);

    auto header = getLocalBounds().reduced (16, 10).removeFromTop (34);
    g.setColour (kInk);
    g.setFont (juce::Font (juce::FontOptions (24.0f)).boldened());
    g.drawText ("Chime", header, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.setColour (juce::Colour (0xff555550));
    g.drawText ("resonator", header, juce::Justification::centredRight);

    // Small +/- marks flanking the attenuverter row.
    g.setFont (juce::Font (juce::FontOptions (12.0f)).boldened());
    g.setColour (juce::Colour (0xff777770));
    for (auto* k : { &frequencyAmount_, &structureAmount_, &brightnessAmount_, &dampingAmount_, &positionAmount_ })
    {
        auto b = k->slider.getBounds();
        g.drawText ("-", b.withX (b.getX() - 14).withWidth (14), juce::Justification::centred);
        g.drawText ("+", b.withX (b.getRight()).withWidth (14), juce::Justification::centred);
    }

    auto badge = getLocalBounds().removeFromBottom (24).reduced (16, 2);
    g.setColour (juce::Colour (0xff777770));
    g.setFont (juce::Font (juce::FontOptions (10.0f)).boldened());
    g.drawText ("SOULMADE", badge, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText ("DSP after Mutable Instruments' Rings (MIT)", badge, juce::Justification::centredRight);
}

void ChimeEditor::resized()
{
    auto area = getLocalBounds().reduced (16, 10);
    area.removeFromTop (38);
    area.removeFromBottom (26);

    // Two big knobs, side by side.
    auto topRow = area.removeFromTop (140);
    const int bigSize = 100;
    auto freqCell = topRow.removeFromLeft (topRow.getWidth() / 2);
    frequency_.slider.setBounds (freqCell.withSizeKeepingCentre (bigSize, bigSize));
    frequency_.label.setBounds (freqCell.withSizeKeepingCentre (bigSize, 16).withY (freqCell.getBottom() - 18));
    auto structCell = topRow;
    structure_.slider.setBounds (structCell.withSizeKeepingCentre (bigSize, bigSize));
    structure_.label.setBounds (structCell.withSizeKeepingCentre (bigSize, 16).withY (structCell.getBottom() - 18));

    freqCentre_ = frequency_.slider.getBounds().getCentre().toFloat();
    structCentre_ = structure_.slider.getBounds().getCentre().toFloat();

    area.removeFromTop (10);

    // Three medium knobs in equal thirds.
    auto midRow = area.removeFromTop (120);
    const int midSize = 80;
    const int thirdW = midRow.getWidth() / 3;
    auto brightCell = midRow.removeFromLeft (thirdW);
    brightness_.slider.setBounds (brightCell.withSizeKeepingCentre (midSize, midSize));
    brightness_.label.setBounds (brightCell.withSizeKeepingCentre (midSize, 16).withY (brightCell.getBottom() - 18));
    auto dampCell = midRow.removeFromLeft (thirdW);
    damping_.slider.setBounds (dampCell.withSizeKeepingCentre (midSize, midSize));
    damping_.label.setBounds (dampCell.withSizeKeepingCentre (midSize, 16).withY (dampCell.getBottom() - 18));
    auto posCell = midRow;
    position_.slider.setBounds (posCell.withSizeKeepingCentre (midSize, midSize));
    position_.label.setBounds (posCell.withSizeKeepingCentre (midSize, 16).withY (posCell.getBottom() - 18));

    brightCentre_ = brightness_.slider.getBounds().getCentre().toFloat();
    dampCentre_ = damping_.slider.getBounds().getCentre().toFloat();
    posCentre_ = position_.slider.getBounds().getCentre().toFloat();

    area.removeFromTop (20); // extra space for the +/- marks drawn in paint()

    // Five small attenuverter trim pots in one row.
    auto trimRow = area.removeFromTop (70);
    Knob* trims[] = { &frequencyAmount_, &structureAmount_, &brightnessAmount_, &dampingAmount_, &positionAmount_ };
    const int tw = trimRow.getWidth() / 5;
    for (auto* k : trims)
    {
        auto cell = trimRow.removeFromLeft (tw).reduced (18, 0);
        k->slider.setBounds (cell.withTrimmedBottom (14));
        k->label.setBounds (cell.removeFromBottom (14));
    }

    area.removeFromTop (14);

    auto utilRow = area.removeFromTop (24);
    modelLabel_.setBounds (utilRow.removeFromLeft (180));
    auto polyArea = utilRow.removeFromRight (120);
    polyLabel_.setBounds (polyArea.removeFromLeft (40));
    strumLabel_.setBounds (utilRow);

    auto controlsRow = area.removeFromTop (28);
    modelBox_.setBounds (controlsRow.removeFromLeft (180));
    controlsRow.removeFromLeft (6);
    auto polyControlArea = controlsRow.removeFromRight (120);
    polySlider_.setBounds (polyControlArea);
    controlsRow.removeFromRight (6);
    strumBox_.setBounds (controlsRow);
}

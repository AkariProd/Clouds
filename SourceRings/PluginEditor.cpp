#include "PluginEditor.h"

namespace
{
const juce::Colour kCyan   (0xff29c7d6);
const juce::Colour kMagenta(0xffd63a8f);
const juce::Colour kAmber  (0xfff2a93c);
const juce::Colour kPanel  (0xff1c1c1c);
const juce::Colour kInk    (0xffffffff);
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

    makeKnob (frequencyAmount_, rng::pid::frequencyAmount, "FM AMT", kCyan);
    makeKnob (structureAmount_, rng::pid::structureAmount, "STR AMT", kMagenta);
    makeKnob (brightnessAmount_, rng::pid::brightnessAmount, "BRI AMT", kAmber);
    makeKnob (dampingAmount_, rng::pid::dampingAmount, "DMP AMT", kMagenta);
    makeKnob (positionAmount_, rng::pid::positionAmount, "POS AMT", kAmber);

    modelBox_.addItemList ({ "Modal Resonator", "Sympathetic Strings", "Modulated/Inharmonic String",
                              "FM Voice", "Sympathetic Strings (Quantized)", "String + Reverb" }, 1);
    strumBox_.addItemList ({ "External Trigger", "Note Change", "Audio Onset" }, 1);

    for (auto* c : { &modelBox_, &strumBox_ })
        addAndMakeVisible (c);
    for (auto* l : { &modelLabel_, &strumLabel_, &polyLabel_ })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::textColourId, juce::Colour (0xffaaaaaa));
        l->setFont (juce::Font (juce::FontOptions (10.0f)).boldened());
        addAndMakeVisible (l);
    }
    addAndMakeVisible (polySlider_);

    modelAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc_.apvts, rng::pid::model, modelBox_);
    strumAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc_.apvts, rng::pid::strumMode, strumBox_);
    polyAtt_  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc_.apvts, rng::pid::polyphony, polySlider_);

    setSize (460, 640);
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

    g.setColour (juce::Colour (0xff3a3a3a));
    for (auto p : { juce::Point<int> (12, 12), juce::Point<int> (getWidth() - 12, 12),
                     juce::Point<int> (12, getHeight() - 12), juce::Point<int> (getWidth() - 12, getHeight() - 12) })
        g.fillEllipse ((float) p.x - 3.0f, (float) p.y - 3.0f, 6.0f, 6.0f);

    // Ring motif behind the five main knobs, echoing the module's own
    // circular graphic.
    auto ringArea = getLocalBounds().reduced (16, 10).withTrimmedTop (36).withHeight (260);
    g.setColour (juce::Colour (0xff2a2a2a));
    g.drawEllipse (ringArea.toFloat().reduced (8.0f), 1.5f);
    g.drawEllipse (ringArea.toFloat().reduced (24.0f), 1.0f);

    auto header = getLocalBounds().reduced (16, 10).removeFromTop (34);
    g.setColour (kInk);
    g.setFont (juce::Font (juce::FontOptions (24.0f)).boldened());
    g.drawText ("Chime", header, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.setColour (juce::Colour (0xff999999));
    g.drawText ("resonator", header, juce::Justification::centredRight);

    auto badge = getLocalBounds().removeFromBottom (24).reduced (16, 2);
    g.setColour (juce::Colour (0xff888888));
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

    // Five main knobs: Frequency centred top, Structure/Brightness either
    // side, Damping/Position below those.
    auto topRow = area.removeFromTop (130);
    const int bigSize = 90;
    frequency_.slider.setBounds (topRow.withSizeKeepingCentre (bigSize, bigSize));
    frequency_.label.setBounds (topRow.withSizeKeepingCentre (bigSize, 16).withY (topRow.getBottom() - 18));

    auto midRow = area.removeFromTop (120);
    auto left1 = midRow.removeFromLeft (midRow.getWidth() / 2);
    structure_.label.setBounds (left1.removeFromTop (16));
    structure_.slider.setBounds (left1);
    brightness_.label.setBounds (midRow.removeFromTop (16));
    brightness_.slider.setBounds (midRow);

    area.removeFromTop (6);
    auto lowRow = area.removeFromTop (120);
    auto left2 = lowRow.removeFromLeft (lowRow.getWidth() / 2);
    damping_.label.setBounds (left2.removeFromTop (16));
    damping_.slider.setBounds (left2);
    position_.label.setBounds (lowRow.removeFromTop (16));
    position_.slider.setBounds (lowRow);

    area.removeFromTop (10);

    // Five small attenuverter trim pots in one row.
    auto trimRow = area.removeFromTop (70);
    Knob* trims[] = { &frequencyAmount_, &structureAmount_, &brightnessAmount_, &dampingAmount_, &positionAmount_ };
    const int tw = trimRow.getWidth() / 5;
    for (auto* k : trims)
    {
        auto cell = trimRow.removeFromLeft (tw);
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

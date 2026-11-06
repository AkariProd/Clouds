#include "PluginEditor.h"
#include "Parameters.h"

namespace
{
// Small accent palette echoing the panel's own cyan/magenta/amber marks.
const juce::Colour kCyan   (0xff29c7d6);
const juce::Colour kMagenta(0xffd63a8f);
const juce::Colour kAmber  (0xfff2a93c);
const juce::Colour kPanel  (0xfff4f3ef);   // off-white panel stock
const juce::Colour kInk    (0xff1a1a1a);
}

PebbleEditor::PebbleEditor (PebbleProcessor& p)
    : juce::AudioProcessorEditor (&p), proc_ (p)
{
    setLookAndFeel (&lnf_);

    makeKnob (density_, bds::pid::density, "DENSITY", kCyan);
    makeKnob (time_,    bds::pid::time,    "TIME");
    makeKnob (pitch_,   bds::pid::pitch,   "PITCH");
    makeKnob (size_,    bds::pid::size,    "SIZE");
    makeKnob (shape_,   bds::pid::shape,   "SHAPE");

    // Seven small trim pots, same order as the hardware's muxed pot bank.
    makeKnob (feedback_,    bds::pid::feedback,    "FDBK",  kMagenta);
    makeKnob (dryWet_,      bds::pid::dryWet,      "D/W",   kMagenta);
    makeKnob (reverb_,      bds::pid::reverb,      "REV",   kMagenta);
    makeKnob (timeAmount_,  bds::pid::timeAmount,  "TIME",  kCyan);
    makeKnob (sizeAmount_,  bds::pid::sizeAmount,  "SIZE",  kCyan);
    makeKnob (shapeAmount_, bds::pid::shapeAmount, "SHAPE", kCyan);
    makeKnob (pitchAmount_, bds::pid::pitchAmount, "PITCH", kAmber);

    for (auto* b : { &freezeBtn_, &seedBtn_ })
        addAndMakeVisible (b);

    for (auto* b : { &seedLockBtn_, &monoGrainBtn_, &outputGateBtn_ })
    {
        b->setColour (juce::ToggleButton::textColourId, kInk);
        addAndMakeVisible (b);
    }

    freezeAtt_     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::freeze, freezeBtn_);
    seedAtt_       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::seed, seedBtn_);
    seedLockAtt_   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::seedLock, seedLockBtn_);
    monoGrainAtt_  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::monoGrain, monoGrainBtn_);
    outputGateAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::outputGate, outputGateBtn_);

    mediaBox_.addItemList ({ "Dirty Tape", "Clean Tape", "Vintage Digital", "Digital" }, 1);
    mediaBox_.setColour (juce::ComboBox::backgroundColourId, juce::Colours::white);
    addAndMakeVisible (mediaBox_);
    addAndMakeVisible (mediaLabel_);
    mediaLabel_.attachToComponent (&mediaBox_, false);
    mediaLabel_.setJustificationType (juce::Justification::centred);
    mediaAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc_.apvts, bds::pid::media, mediaBox_);

    setSize (480, 760);
}

PebbleEditor::~PebbleEditor()
{
    setLookAndFeel (nullptr);
}

void PebbleEditor::makeKnob (Knob& k, const juce::String& paramId, const juce::String& text, juce::Colour accent)
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

void PebbleEditor::drawJackRow (juce::Graphics& g, juce::Rectangle<int> area, std::initializer_list<juce::String> labels)
{
    const int n = (int) labels.size();
    if (n == 0)
        return;
    const int w = area.getWidth() / n;
    int i = 0;
    for (const auto& label : labels)
    {
        auto cell = area.withX (area.getX() + i * w).withWidth (w);
        const float d = (float) juce::jmin (w - 10, 26);
        auto circle = juce::Rectangle<float> (d, d).withCentre (cell.getCentre().toFloat());

        g.setColour (juce::Colour (0xffcfcfc8));
        g.fillEllipse (circle);
        g.setColour (juce::Colour (0xff8a8a82));
        g.drawEllipse (circle, 1.2f);
        g.setColour (juce::Colour (0xff555550));
        g.fillEllipse (circle.reduced (d * 0.32f));

        g.setColour (kInk);
        g.setFont (juce::Font (juce::FontOptions (9.0f)));
        g.drawText (label, cell.withY (cell.getBottom() - 14).withHeight (14),
                    juce::Justification::centred);
        ++i;
    }
}

void PebbleEditor::paint (juce::Graphics& g)
{
    g.fillAll (kPanel);

    // Four corner mounting-screw dots, like a real Eurorack faceplate.
    g.setColour (juce::Colour (0xffb5b5ac));
    for (auto p : { juce::Point<int> (12, 12), juce::Point<int> (getWidth() - 12, 12),
                     juce::Point<int> (12, getHeight() - 12), juce::Point<int> (getWidth() - 12, getHeight() - 12) })
        g.fillEllipse ((float) p.x - 3.0f, (float) p.y - 3.0f, 6.0f, 6.0f);

    auto header = getLocalBounds().reduced (16, 10).removeFromTop (34);
    g.setColour (kInk);
    g.setFont (juce::Font (juce::FontOptions (24.0f)).boldened());
    g.drawText ("Pebble", header, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.setColour (juce::Colour (0xff555550));
    g.drawText ("texture synthesizer", header, juce::Justification::centredRight);

    drawJackRow (g, cvJackRowArea_, { "ASSIGN", "DENSITY", "PITCH", "TIME", "SIZE", "SHAPE" });
    g.setColour (juce::Colour (0xff999992));
    g.setFont (juce::Font (juce::FontOptions (9.0f)).boldened());
    g.drawText ("CV IN", cvJackRowArea_.withY (cvJackRowArea_.getY() - 12).withHeight (12),
                juce::Justification::centredLeft);

    drawJackRow (g, audioJackRowArea_, { "IN L", "IN R", "FREEZE", "SEED", "L", "R" });
    g.drawText ("I/O", audioJackRowArea_.withY (audioJackRowArea_.getY() - 12).withHeight (12),
                juce::Justification::centredLeft);

    // Original vendor mark (not a copy of Mutable Instruments' logo).
    auto badge = getLocalBounds().removeFromBottom (24).reduced (16, 2);
    g.setColour (juce::Colour (0xff777770));
    g.setFont (juce::Font (juce::FontOptions (10.0f)).boldened());
    g.drawText ("SOULMADE", badge, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText ("DSP after Mutable Instruments' Beads (MIT)", badge, juce::Justification::centredRight);
}

void PebbleEditor::resized()
{
    auto area = getLocalBounds().reduced (16, 10);
    area.removeFromTop (38); // header
    area.removeFromBottom (26); // vendor badge

    // FREEZE / SEED at the top corners, same as hardware.
    auto topRow = area.removeFromTop (56);
    freezeBtn_.setBounds (topRow.removeFromLeft (70).reduced (4));
    seedBtn_.setBounds (topRow.removeFromRight (70).reduced (4));
    area.removeFromTop (6);

    // Big DENSITY knob, centred.
    auto densityRow = area.removeFromTop (130);
    const int densitySize = 110;
    density_.slider.setBounds (densityRow.withSizeKeepingCentre (densitySize, densitySize));
    density_.label.setBounds (densityRow.withSizeKeepingCentre (densitySize, 16).withY (densityRow.getBottom() - 18));
    area.removeFromTop (4);

    auto pairRow = [&] (Knob& l, Knob& r, int height)
    {
        auto row = area.removeFromTop (height);
        auto leftCell = row.removeFromLeft (row.getWidth() / 2);
        auto rightCell = row;
        l.label.setBounds (leftCell.removeFromTop (16));
        l.slider.setBounds (leftCell);
        r.label.setBounds (rightCell.removeFromTop (16));
        r.slider.setBounds (rightCell);
    };

    pairRow (time_, pitch_, 100);
    area.removeFromTop (4);
    pairRow (size_, shape_, 100);
    area.removeFromTop (10);

    // Seven small trim pots in one row, matching the hardware's muxed bank.
    auto trimRow = area.removeFromTop (76);
    Knob* trims[] = { &feedback_, &dryWet_, &reverb_, &timeAmount_, &sizeAmount_, &shapeAmount_, &pitchAmount_ };
    const int tw = trimRow.getWidth() / 7;
    for (auto* k : trims)
    {
        auto cell = trimRow.removeFromLeft (tw);
        k->slider.setBounds (cell.withTrimmedBottom (14));
        k->label.setBounds (cell.removeFromBottom (14));
    }
    area.removeFromTop (14);

    // Decorative CV jack row (visual only — see header comment).
    cvJackRowArea_ = area.removeFromTop (42);
    area.removeFromTop (16);

    // Decorative audio I/O jack row.
    audioJackRowArea_ = area.removeFromTop (42);
    area.removeFromTop (10);

    // Utility row: controls the real hardware reaches via button-holds
    // rather than a dedicated knob (quality/media, grain mode, gate out,
    // seed lock) — kept compact and visually secondary.
    auto utilRow = area.removeFromTop (28);
    mediaBox_.setBounds (utilRow.removeFromLeft (140));
    utilRow.removeFromLeft (8);
    seedLockBtn_.setBounds (utilRow.removeFromLeft (60));
    monoGrainBtn_.setBounds (utilRow.removeFromLeft (70));
    outputGateBtn_.setBounds (utilRow.removeFromLeft (60));
}

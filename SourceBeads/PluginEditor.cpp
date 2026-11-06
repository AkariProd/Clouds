#include "PluginEditor.h"
#include "Parameters.h"

PebbleEditor::PebbleEditor (PebbleProcessor& p)
    : juce::AudioProcessorEditor (&p), proc_ (p)
{
    makeKnob (time_,   bds::pid::time,   "Time");
    makeKnob (density_,bds::pid::density,"Density");
    makeKnob (pitch_,  bds::pid::pitch,  "Pitch");
    makeKnob (size_,   bds::pid::size,   "Size");
    makeKnob (shape_,  bds::pid::shape,  "Shape");

    makeKnob (timeAmount_,  bds::pid::timeAmount,  "Time Amt");
    makeKnob (pitchAmount_, bds::pid::pitchAmount, "Pitch Amt");
    makeKnob (sizeAmount_,  bds::pid::sizeAmount,  "Size Amt");
    makeKnob (shapeAmount_, bds::pid::shapeAmount, "Shape Amt");

    makeKnob (feedback_, bds::pid::feedback, "Feedback");
    makeKnob (dryWet_,   bds::pid::dryWet,   "Dry/Wet");
    makeKnob (reverb_,   bds::pid::reverb,   "Reverb");

    for (auto* b : { &freezeBtn_, &seedBtn_, &seedLockBtn_, &monoGrainBtn_, &outputGateBtn_ })
        addAndMakeVisible (b);

    freezeAtt_     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::freeze, freezeBtn_);
    seedAtt_       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::seed, seedBtn_);
    seedLockAtt_   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::seedLock, seedLockBtn_);
    monoGrainAtt_  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::monoGrain, monoGrainBtn_);
    outputGateAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc_.apvts, bds::pid::outputGate, outputGateBtn_);

    mediaBox_.addItemList ({ "Dirty Tape", "Clean Tape", "Vintage Digital", "Digital" }, 1);
    addAndMakeVisible (mediaBox_);
    addAndMakeVisible (mediaLabel_);
    mediaLabel_.attachToComponent (&mediaBox_, false);
    mediaLabel_.setJustificationType (juce::Justification::centred);
    mediaAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc_.apvts, bds::pid::media, mediaBox_);

    setSize (700, 420);
}

void PebbleEditor::makeKnob (Knob& k, const juce::String& paramId, const juce::String& text)
{
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible (k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc_.apvts, paramId, k.slider);
}

void PebbleEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff202225));

    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawText ("Pebble", getLocalBounds().removeFromTop (28),
                juce::Justification::centred);

    g.setFont (12.0f);
    g.setColour (juce::Colours::grey);
    g.drawText ("a texture synthesizer, after Mutable Instruments' Beads (MIT-licensed DSP)",
                getLocalBounds().removeFromBottom (16), juce::Justification::centred);
}

void PebbleEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (28);
    area.removeFromBottom (16);

    auto knobRow = [&] (std::initializer_list<Knob*> knobs, int height)
    {
        auto row = area.removeFromTop (height);
        const int w = row.getWidth() / (int) knobs.size();
        for (auto* k : knobs)
        {
            auto cell = row.removeFromLeft (w);
            k->label.setBounds (cell.removeFromTop (16));
            k->slider.setBounds (cell);
        }
    };

    knobRow ({ &time_, &density_, &pitch_, &size_, &shape_ }, 120);
    area.removeFromTop (6);
    knobRow ({ &timeAmount_, &pitchAmount_, &sizeAmount_, &shapeAmount_ }, 100);
    area.removeFromTop (6);
    knobRow ({ &feedback_, &dryWet_, &reverb_ }, 100);
    area.removeFromTop (12);

    auto buttonsRow = area.removeFromTop (40);
    const int bw = buttonsRow.getWidth() / 6;
    freezeBtn_.setBounds (buttonsRow.removeFromLeft (bw).reduced (4));
    seedBtn_.setBounds (buttonsRow.removeFromLeft (bw).reduced (4));
    seedLockBtn_.setBounds (buttonsRow.removeFromLeft (bw).reduced (4));
    monoGrainBtn_.setBounds (buttonsRow.removeFromLeft (bw).reduced (4));
    outputGateBtn_.setBounds (buttonsRow.removeFromLeft (bw).reduced (4));
    mediaBox_.setBounds (buttonsRow.reduced (4, 10));
}

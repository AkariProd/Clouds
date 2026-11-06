#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "PebbleLookAndFeel.h"

/** Panel recreates the hardware's actual control grid and colour accents —
    white panel, black knobs, big DENSITY knob flanked by TIME/PITCH then
    SIZE/SHAPE, a row of seven small trim pots underneath (Feedback, Dry/Wet,
    Reverb, then the four CV-amount attenuverters), FREEZE/SEED at the top
    corners. No decorative jack rows — a VST has no patch cables, so fake
    jacks that look clickable but aren't were actively confusing rather than
    just decorative; removed rather than kept as dead weight. Branded
    "Pebble" / "SoulMade" rather than the original names, per Mutable
    Instruments' own guidance for derivative works; their CC-BY-SA-licensed
    panel layout is recreated, their trademarked logo is not — the bottom
    badge is an original mark, not a copy of theirs. */
class PebbleEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PebbleEditor (PebbleProcessor&);
    ~PebbleEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void makeKnob (Knob& k, const juce::String& paramId, const juce::String& text, juce::Colour accent = {});

    PebbleProcessor& proc_;
    bds::PebbleLookAndFeel lnf_;

    Knob density_;
    Knob time_, pitch_;
    Knob size_, shape_;
    Knob feedback_, dryWet_, reverb_, timeAmount_, sizeAmount_, shapeAmount_, pitchAmount_;

    juce::ToggleButton freezeBtn_ { "FREEZE" };
    juce::ToggleButton seedBtn_ { "SEED" };

    juce::ComboBox mediaBox_;
    juce::Label mediaLabel_ { {}, "MEDIA" };
    juce::ToggleButton seedLockBtn_ { "LOCK" };
    juce::ToggleButton monoGrainBtn_ { "MONO" };
    juce::ToggleButton outputGateBtn_ { "GATE" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAtt_, seedAtt_, seedLockAtt_, monoGrainAtt_, outputGateAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mediaAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PebbleEditor)
};

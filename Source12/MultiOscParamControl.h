#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include <array>

// One "shared" oscillator knob/toggle/combo -- e.g. "Pitch" -- that edits
// whichever of the 4 oscillators are currently selected via the OSC 1-4
// buttons, mirroring how the real Prophet 12 panel reuses one row of
// oscillator controls across all 4 oscillators via an OSC SELECT.
//
// Turning this knob calls setValueNotifyingHost() on every selected
// oscillator's underlying (separate, independently-automatable) APVTS
// parameter with the same value -- so e.g. selecting OSC 1 + OSC 3 and
// turning "Pitch" sets both to the same new pitch in one move. The display
// always reflects the lowest-numbered currently-selected oscillator.
class MultiOscParamControl : public juce::Component, private juce::Timer
{
public:
    // defsPerOsc[i] is the ParamDef for oscillator (i+1)'s version of this
    // field, or nullptr if that oscillator doesn't have it (e.g. only Osc 1
    // has "Sub Octave").
    MultiOscParamControl(juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& fieldLabel,
                         std::array<const p12::ParamDef*, 4> defsPerOsc,
                         const std::array<bool, 4>& selection);

    void resized() override;
    void paint(juce::Graphics& g) override;

    // Called by the owning section right after the OSC 1-4 selection
    // changes, so the display jumps to the newly-relevant oscillator at
    // once rather than waiting for the next timer tick.
    void refreshFromSelection();

    static constexpr int kWidth = 74;
    static constexpr int kHeight = 78;

private:
    void timerCallback() override { refreshFromSelection(); }
    int firstSelectedWithDef() const;
    void applyToSelected(float realValue);

    juce::AudioProcessorValueTreeState& apvts;
    std::array<const p12::ParamDef*, 4> defs;
    const std::array<bool, 4>& selection;
    const p12::ParamDef* representativeDef = nullptr;  // first non-null def, for range/type

    juce::Label nameLabel;
    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::ToggleButton> toggle;
    std::unique_ptr<juce::ComboBox> combo;

    bool updatingFromCode = false;
};

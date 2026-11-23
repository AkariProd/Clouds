#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "CategorizedCombo.h"
#include <vector>

// Generalized version of what OscillatorSection introduced: ONE "shared"
// knob/toggle/combo (e.g. "Attack") that edits whichever of N numbered
// instances (oscillators, LFOs, delays, envelopes, mod-matrix slots...) are
// currently selected via that section's own select buttons -- mirroring how
// the Prophet 12's own panel reuses one row of controls across multiple
// numbered instances instead of giving each its own full knob row.
//
// Turning this control calls setValueNotifyingHost() on every selected
// instance's underlying (separate, independently-automatable) APVTS
// parameter with the same value. The display always reflects the
// lowest-indexed currently-selected instance.
class MultiInstanceParamControl : public juce::Component, private juce::Timer
{
public:
    // defsPerInstance[i] is the ParamDef for instance i's version of this
    // field, or nullptr if that instance doesn't have it (e.g. only Envelope
    // 3/4 have a "Destination" field; VCA/LPF envelopes don't apply here).
    MultiInstanceParamControl(juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& fieldLabel,
                               std::vector<const p12::ParamDef*> defsPerInstance,
                               const std::vector<bool>& selection);

    void resized() override;
    void paint(juce::Graphics& g) override;

    // Called by the owning section right after the select-button state
    // changes, so the display jumps to the newly-relevant instance at once
    // rather than waiting for the next timer tick.
    void refreshFromSelection();

    static constexpr int kWidth = 74;
    static constexpr int kHeight = 78;

private:
    void timerCallback() override { refreshFromSelection(); }
    int firstSelectedWithDef() const;
    void applyToSelected(float realValue);

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<const p12::ParamDef*> defs;
    const std::vector<bool>& selection;
    const p12::ParamDef* representativeDef = nullptr;  // first non-null def, for range/type

    juce::Label nameLabel;
    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::ToggleButton> toggle;
    std::unique_ptr<juce::ComboBox> combo;
    std::unique_ptr<CategorizedCombo> treeCombo;  // used instead of `combo` when categorized

    bool updatingFromCode = false;
};

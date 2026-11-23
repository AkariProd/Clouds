#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MultiOscParamControl.h"
#include "PanelLayoutable.h"

// Replaces the 4 separate "Oscillator 1".."Oscillator 4" SectionPanels with
// ONE oscillator section: OSC 1-4 select buttons (multi-select, like the
// Prophet 12's own panel) plus a single shared row of knobs that edit
// whichever oscillator(s) are currently selected -- so e.g. selecting
// OSC 1 + OSC 3 and turning "Pitch" tunes both at once instead of needing
// to repeat the same tweak four times.
class OscillatorSection : public juce::Component, public PanelLayoutable
{
public:
    OscillatorSection(const juce::String& title, juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics&) override;
    void resized() override;

    int getPreferredWidth() const override { return preferredWidth; }
    int getPreferredHeight() const override { return preferredHeight; }

private:
    void toggleOsc(int index);

    juce::String titleText;
    std::array<bool, 4> selection { true, false, false, false };
    std::array<std::unique_ptr<juce::TextButton>, 4> oscButtons;
    juce::OwnedArray<MultiOscParamControl> controls;

    int columns = 5;
    int preferredWidth = 0;
    int preferredHeight = 0;

    static constexpr int kHeaderHeight = 22;
    static constexpr int kSelectRowHeight = 28;
    static constexpr int kPadding = 6;
};

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MultiInstanceParamControl.h"
#include "PanelLayoutable.h"

// Generalized version of what OscillatorSection introduced: groups N
// numbered instances of the same kind of thing (oscillators, LFOs, delays,
// envelopes, mod-matrix slots) into ONE section with N select buttons
// (multi-select) and one shared row of knobs/switches that edits whichever
// instance(s) are currently selected -- mirroring the Prophet 12's own
// panel, which reuses one row of controls across several numbered
// instances via a SELECT rather than giving each its own full knob row.
//
// Field grouping is derived automatically from the generated parameter
// names: for each `instanceNumbers[i]`, every namePrefix is tried as
// "<prefix> <N> " searched ANYWHERE in the parameter name (not just as a
// strict prefix -- needed for names like "Velocity to Env 3 Amt" where the
// instance number isn't at the start). The matched substring is stripped
// out to get a field label shared across instances (e.g. "Envelope 3 Amt"
// and "Envelope 4 Amt" -> "Amt").
class MultiInstanceSection : public juce::Component, public PanelLayoutable
{
public:
    MultiInstanceSection(const juce::String& title, juce::AudioProcessorValueTreeState& apvts,
                          const std::vector<int>& instanceNumbers,
                          const std::vector<juce::String>& namePrefixes,
                          const juce::String& buttonPrefix);

    void paint(juce::Graphics&) override;
    void resized() override;

    int getPreferredWidth() const override { return preferredWidth; }
    int getPreferredHeight() const override { return preferredHeight; }

private:
    void toggleInstance(int index);

    juce::String titleText;
    int numInstances = 0;
    std::vector<bool> selection;
    std::vector<std::unique_ptr<juce::TextButton>> instanceButtons;
    juce::OwnedArray<MultiInstanceParamControl> controls;

    int columns = 5;
    int buttonColumns = 4;
    int buttonRows = 1;
    int preferredWidth = 0;
    int preferredHeight = 0;

    static constexpr int kHeaderHeight = 22;
    static constexpr int kSelectRowHeight = 26;
    static constexpr int kPadding = 6;
};

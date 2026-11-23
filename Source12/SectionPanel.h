#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ParamControl.h"
#include "PanelLayoutable.h"

// A titled box ("OSCILLATOR 1", "MODULATION MATRIX", ...) holding a wrapped
// grid of ParamControls -- one per NRPN parameter in that section.
class SectionPanel : public juce::Component, public PanelLayoutable
{
public:
    SectionPanel(const juce::String& title, juce::AudioProcessorValueTreeState& apvts,
                 const std::vector<const p12::ParamDef*>& defs);

    void paint(juce::Graphics&) override;
    void resized() override;

    int getPreferredWidth() const override { return preferredWidth; }
    int getPreferredHeight() const override { return preferredHeight; }

private:
    juce::String titleText;
    juce::OwnedArray<ParamControl> controls;
    int columns = 4;
    int preferredWidth = 0;
    int preferredHeight = 0;

    static constexpr int kHeaderHeight = 22;
    static constexpr int kPadding = 6;
};

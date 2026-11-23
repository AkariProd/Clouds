#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "SectionPanel.h"
#include "MultiInstanceSection.h"
#include "LookAndFeelP12.h"
#include "MidiMonitorPanel.h"
#include "SysexCapturePanel.h"
#include "HardwareSyncPanel.h"

// The content Component that actually holds every section, scrolled
// inside the editor's Viewport.
class Seer12PanelContent : public juce::Component
{
public:
    Seer12PanelContent(juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
    void paint(juce::Graphics&) override;

private:
    // Owns both kinds of section; laid out together via PanelLayoutable so
    // the flow-wrap code doesn't need to care which is which.
    juce::OwnedArray<juce::Component> ownedSections;
    std::vector<PanelLayoutable*> layoutPanels;
    static constexpr int kContentWidth = 1180;
    static constexpr int kMargin = 10;
};

class Seer12Editor : public juce::AudioProcessorEditor
{
public:
    explicit Seer12Editor(Seer12Processor&);
    ~Seer12Editor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    Seer12Processor& processor;
    LookAndFeelP12 lookAndFeel;
    juce::Viewport viewport;
    Seer12PanelContent content;
    juce::Label titleLabel;
    MidiMonitorPanel midiMonitor;
    HardwareSyncPanel hardwareSync;
    SysexCapturePanel sysexCapture;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Seer12Editor)
};

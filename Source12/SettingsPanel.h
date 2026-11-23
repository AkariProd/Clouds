#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "HardwareSyncPanel.h"
#include "MidiMonitorPanel.h"
#include "SysexCapturePanel.h"

// Collapsible drawer holding everything that isn't part of the normal
// playing experience: manual "Sync Now", the raw MIDI activity log, and
// the SysEx dump-capture tool used to reverse-engineer the hardware's
// byte layout. Hidden by default -- toggled from TopBar's gear button.
class SettingsPanel : public juce::Component
{
public:
    explicit SettingsPanel(Seer12Processor& processor);

    void resized() override;
    void paint(juce::Graphics&) override;

    static constexpr int kPreferredHeight = 112 + 60 + 64 + 40;

private:
    juce::Label titleLabel;
    HardwareSyncPanel hardwareSync;
    MidiMonitorPanel midiMonitor;
    SysexCapturePanel sysexCapture;
};

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// End-user-facing hardware sync control: pulls a full Program Edit Buffer
// Dump and applies every mapped byte to the matching on-screen control in
// one go (see Seer12Processor::applyFullDump / Source12/tools/byte_offset_map.json).
// Also fires automatically when the hardware sends a Program Change, so
// recalling a preset on the P12 updates every knob here too, not just
// whichever one you happen to touch next.
class HardwareSyncPanel : public juce::Component, private juce::Timer
{
public:
    explicit HardwareSyncPanel(Seer12Processor& processor);

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    void timerCallback() override;

    Seer12Processor& proc;
    juce::TextButton syncButton { "Sync Now (pull full preset from hardware)" };
    juce::Label statusLabel;

    int lastSeenSequence = -1;
};

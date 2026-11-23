#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// Always-visible diagnostic strip: total MIDI CC messages seen, how many
// the NRPN decoder resolved into a parameter update, a blinking activity
// LED, and a short scrolling log of the last messages -- so "is MIDI
// reaching the plugin at all" is answerable by looking at the window
// instead of guessing from silence.
class MidiMonitorPanel : public juce::Component, private juce::Timer
{
public:
    explicit MidiMonitorPanel(Seer12Processor& p);
    ~MidiMonitorPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    Seer12Processor& processor;
    int lastSeenTotalReceived = 0;
    int ledFlashCountdown = 0;

    juce::Label statusLabel;
    juce::TextEditor logBox;

    static constexpr int kHeight = 92;
};

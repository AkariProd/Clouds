#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// Byte-layout reverse-engineering tool: asks the hardware for a full
// Program Edit Buffer Dump (undocumented byte layout -- see
// Source12/CONTEXT.md, "SysEx dump capture") and saves each one to disk
// with a short label, so dumps taken before/after changing one parameter
// can be diffed offline to work out which byte(s) that parameter lives at.
class SysexCapturePanel : public juce::Component, private juce::Timer
{
public:
    explicit SysexCapturePanel(Seer12Processor& processor);

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    void timerCallback() override;
    void captureClicked();

    Seer12Processor& proc;
    juce::Label title;
    juce::TextEditor tagBox;
    juce::TextButton captureButton { "Request + Capture Dump" };
    juce::Label statusLabel;

    int lastSeenSequence = 0;
    int captureCounter = 0;
    juce::File captureDir;
};

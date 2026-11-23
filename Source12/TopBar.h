#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// Header bar: title, a preset-name viewer with prev/next (sends real
// Program Change + pulls a fresh sync), and a gear button that toggles the
// settings drawer (MIDI monitor / SysEx capture / manual Sync Now -- see
// SettingsPanel). Visual language borrows the "preset browser strip" every
// major soft-synth (Arturia, u-he, GForce...) uses, not any one vendor's.
class TopBar : public juce::Component, private juce::Timer
{
public:
    TopBar(Seer12Processor& processor);

    void resized() override;
    void paint(juce::Graphics&) override;

    std::function<void()> onSettingsClicked;

private:
    void timerCallback() override;

    Seer12Processor& proc;
    juce::Label titleLabel;
    juce::Label presetLabel;
    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton settingsButton { juce::String::fromUTF8("\xE2\x9A\x99") };  // gear glyph

    int lastSeenSequence = -1;
};

#include "HardwareSyncPanel.h"
#include "LookAndFeelP12.h"

HardwareSyncPanel::HardwareSyncPanel(Seer12Processor& processor) : proc(processor)
{
    syncButton.onClick = [this] { proc.requestEditBufferDump(); statusLabel.setText("Requesting...", juce::dontSendNotification); };
    addAndMakeVisible(syncButton);

    reconnectButton.onClick = [this]
    {
        proc.directLink.autoConnect();
        linkStatusLabel.setText(proc.directLink.statusText(), juce::dontSendNotification);
    };
    addAndMakeVisible(reconnectButton);

    linkStatusLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    linkStatusLabel.setColour(juce::Label::textColourId, LookAndFeelP12::textColour.withAlpha(0.7f));
    linkStatusLabel.setText(proc.directLink.statusText(), juce::dontSendNotification);
    addAndMakeVisible(linkStatusLabel);

    statusLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    statusLabel.setColour(juce::Label::textColourId, LookAndFeelP12::textColour.withAlpha(0.85f));
    statusLabel.setText("Not synced yet. Click Sync Now, or change presets on the hardware "
                         "(needs GLOBAL > MIDI Program Change Send = On).",
                         juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    lastSeenSequence = proc.fullSyncSequence.load();
    startTimerHz(5);
}

void HardwareSyncPanel::timerCallback()
{
    linkStatusLabel.setText(proc.directLink.statusText(), juce::dontSendNotification);

    const int seq = proc.fullSyncSequence.load();
    if (seq == lastSeenSequence)
        return;
    lastSeenSequence = seq;

    const int n = proc.fullSyncAppliedCount.load();
    const auto name = proc.getLastSyncedPatchName();
    juce::String status = "Synced " + juce::String(n) + " parameters from hardware";
    if (name.isNotEmpty())
        status << "  --  patch: \"" << name << "\"";
    statusLabel.setText(status, juce::dontSendNotification);
}

void HardwareSyncPanel::paint(juce::Graphics& g)
{
    g.setColour(LookAndFeelP12::panelBg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
}

void HardwareSyncPanel::resized()
{
    auto b = getLocalBounds().reduced(8);
    auto top = b.removeFromTop(26);
    syncButton.setBounds(top.removeFromLeft(280));
    top.removeFromLeft(8);
    reconnectButton.setBounds(top.removeFromLeft(100));
    top.removeFromLeft(8);
    linkStatusLabel.setBounds(top);
    b.removeFromTop(4);
    statusLabel.setBounds(b);
}

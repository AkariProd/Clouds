#include "TopBar.h"
#include "LookAndFeelP12.h"

TopBar::TopBar(Seer12Processor& processor) : proc(processor)
{
    titleLabel.setText("SEER 12", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, LookAndFeelP12::accent);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    presetLabel.setText("No preset synced", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions(14.0f)));
    presetLabel.setColour(juce::Label::textColourId, LookAndFeelP12::textColour);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    prevButton.onClick = [this] { proc.changeProgram(-1); };
    nextButton.onClick = [this] { proc.changeProgram(1); };
    addAndMakeVisible(prevButton);
    addAndMakeVisible(nextButton);

    settingsButton.setTooltip("Settings (MIDI monitor, SysEx tools, manual sync)");
    settingsButton.onClick = [this] { if (onSettingsClicked) onSettingsClicked(); };
    addAndMakeVisible(settingsButton);

    lastSeenSequence = proc.fullSyncSequence.load();
    startTimerHz(5);
}

void TopBar::timerCallback()
{
    const int seq = proc.fullSyncSequence.load();
    if (seq == lastSeenSequence)
        return;
    lastSeenSequence = seq;

    const auto name = proc.getLastSyncedPatchName();
    presetLabel.setText(name.isNotEmpty() ? name : "(unnamed)", juce::dontSendNotification);
}

void TopBar::paint(juce::Graphics& g)
{
    g.fillAll(LookAndFeelP12::headerBg);
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawLine(0.0f, (float) getHeight() - 1.0f, (float) getWidth(), (float) getHeight() - 1.0f, 1.0f);

    // Preset-name "well" background, like a recessed LCD strip
    auto presetBounds = presetLabel.getBounds().toFloat().expanded(2.0f);
    g.setColour(LookAndFeelP12::panelBgDark);
    g.fillRoundedRectangle(presetBounds, 4.0f);
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawRoundedRectangle(presetBounds, 4.0f, 1.0f);
}

void TopBar::resized()
{
    auto b = getLocalBounds().reduced(10, 6);
    titleLabel.setBounds(b.removeFromLeft(140));

    settingsButton.setBounds(b.removeFromRight(32));
    b.removeFromRight(10);

    auto presetArea = b.withSizeKeepingCentre(juce::jmin(360, b.getWidth()), b.getHeight());
    prevButton.setBounds(presetArea.removeFromLeft(28));
    nextButton.setBounds(presetArea.removeFromRight(28));
    presetArea.reduce(4, 0);
    presetLabel.setBounds(presetArea);
}

#include "SettingsPanel.h"
#include "LookAndFeelP12.h"

SettingsPanel::SettingsPanel(Seer12Processor& processor)
    : hardwareSync(processor), midiMonitor(processor), sysexCapture(processor)
{
    titleLabel.setText("Settings", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, LookAndFeelP12::accent);
    addAndMakeVisible(titleLabel);

    addAndMakeVisible(hardwareSync);
    addAndMakeVisible(midiMonitor);
    addAndMakeVisible(sysexCapture);
}

void SettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(LookAndFeelP12::panelBgDark.darker(0.15f));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawLine(0.0f, (float) getHeight() - 1.0f, (float) getWidth(), (float) getHeight() - 1.0f, 1.0f);
}

void SettingsPanel::resized()
{
    auto b = getLocalBounds().reduced(8, 6);
    titleLabel.setBounds(b.removeFromTop(20));
    b.removeFromTop(2);
    hardwareSync.setBounds(b.removeFromTop(80));
    b.removeFromTop(4);
    midiMonitor.setBounds(b.removeFromTop(92));
    b.removeFromTop(4);
    sysexCapture.setBounds(b.removeFromTop(64));
}

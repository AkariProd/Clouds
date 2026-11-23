#include "MidiMonitorPanel.h"
#include "LookAndFeelP12.h"

MidiMonitorPanel::MidiMonitorPanel(Seer12Processor& p) : processor(p)
{
    statusLabel.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    statusLabel.setColour(juce::Label::textColourId, LookAndFeelP12::textColour);
    statusLabel.setText("MIDI: waiting for input...", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    logBox.setMultiLine(true);
    logBox.setReadOnly(true);
    logBox.setScrollbarsShown(true);
    logBox.setCaretVisible(false);
    logBox.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::plain)));
    logBox.setColour(juce::TextEditor::backgroundColourId, LookAndFeelP12::panelBgDark);
    logBox.setColour(juce::TextEditor::textColourId, LookAndFeelP12::textColour);
    logBox.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(logBox);

    startTimerHz(20);
}

MidiMonitorPanel::~MidiMonitorPanel()
{
    stopTimer();
}

void MidiMonitorPanel::paint(juce::Graphics& g)
{
    g.setColour(LookAndFeelP12::panelBg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    // Activity LED
    const bool flashing = ledFlashCountdown > 0;
    g.setColour(flashing ? juce::Colours::limegreen : LookAndFeelP12::knobCap.darker(0.3f));
    g.fillEllipse(10.0f, 10.0f, 14.0f, 14.0f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawEllipse(10.0f, 10.0f, 14.0f, 14.0f, 1.0f);
}

void MidiMonitorPanel::resized()
{
    auto b = getLocalBounds().reduced(8);
    b.removeFromLeft(26);  // room for the LED
    statusLabel.setBounds(b.removeFromTop(20));
    b.removeFromTop(4);
    logBox.setBounds(b);
}

void MidiMonitorPanel::timerCallback()
{
    const int total = processor.midiLog.getTotalReceived();
    const int decoded = processor.midiLog.getTotalDecoded();
    const int sentOut = processor.midiLog.getTotalSentOut();

    if (total != lastSeenTotalReceived)
    {
        ledFlashCountdown = 4;  // ~200ms at 20Hz
        lastSeenTotalReceived = total;
    }
    else if (ledFlashCountdown > 0)
    {
        --ledFlashCountdown;
    }

    juce::String status;
    if (total == 0 && sentOut == 0)
        status = "MIDI: no CC messages received yet -- check Options > Audio/MIDI Settings, "
                  "and that the Prophet 12's GLOBAL > 7. MIDI Param Send is set to NRPN.";
    else
        status = juce::String("MIDI in: ") + juce::String(total) + " CC, " + juce::String(decoded)
                  + " decoded.   MIDI out: " + juce::String(sentOut) + " NRPN messages sent.";
    statusLabel.setText(status, juce::dontSendNotification);

    p12::MidiLogEntry entries[p12::MidiActivityLog::kCapacity];
    const int n = processor.midiLog.readRecent(entries, 16);
    juce::String log;
    for (int i = 0; i < n; ++i)
    {
        const auto& e = entries[i];
        log << "CH" << e.channel << "  CC" << juce::String(e.cc).paddedLeft(' ', 3) << " = "
            << juce::String(e.value).paddedLeft(' ', 3);
        if (e.isNrpnDecoded)
            log << "   -> " << p12::kParams[e.paramIndex].name << " = " << e.decodedValue;
        log << "\n";
    }
    if (n == 0)
        log = "(no MIDI yet)";
    logBox.setText(log, juce::dontSendNotification);

    repaint();
}

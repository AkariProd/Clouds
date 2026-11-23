#include "SysexCapturePanel.h"
#include "LookAndFeelP12.h"

SysexCapturePanel::SysexCapturePanel(Seer12Processor& processor) : proc(processor)
{
    captureDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                     .getChildFile("Seer12Captures");
    captureDir.createDirectory();

    // Pick up where a previous session left off, so repeated captures don't
    // overwrite each other across plugin reloads.
    for (auto& f : captureDir.findChildFiles(juce::File::findFiles, false, "*.bin"))
    {
        const int n = f.getFileNameWithoutExtension().upToFirstOccurrenceOf("_", false, false).getIntValue();
        captureCounter = juce::jmax(captureCounter, n);
    }

    title.setText("SysEx Dump Capture (hardware byte-layout mapping)", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, LookAndFeelP12::textColour);
    addAndMakeVisible(title);

    tagBox.setTextToShowWhenEmpty("label this capture, e.g. osc1_pitch_min", juce::Colours::grey);
    tagBox.setFont(juce::Font(juce::FontOptions(12.0f)));
    addAndMakeVisible(tagBox);

    captureButton.onClick = [this] { captureClicked(); };
    addAndMakeVisible(captureButton);

    statusLabel.setText("Set GLOBAL > MIDI Sysex Enable = On and pick a Sysex cable on the P12 first.",
                         juce::dontSendNotification);
    statusLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    statusLabel.setColour(juce::Label::textColourId, LookAndFeelP12::textColour.withAlpha(0.8f));
    addAndMakeVisible(statusLabel);

    startTimerHz(10);
}

void SysexCapturePanel::captureClicked()
{
    statusLabel.setText("Requesting dump...", juce::dontSendNotification);
    proc.requestEditBufferDump();
}

void SysexCapturePanel::timerCallback()
{
    std::vector<uint8_t> data;
    bool isEditBuffer = false;
    int bank = -1, program = -1, seq = 0;
    if (!proc.sysexLog.takeIfNew(lastSeenSequence, data, isEditBuffer, bank, program, seq))
        return;
    lastSeenSequence = seq;

    ++captureCounter;
    juce::String tag = tagBox.getText().trim();
    if (tag.isEmpty())
        tag = "untagged";
    tag = tag.replaceCharacters(" /\\:", "____");

    const auto fileName = juce::String::formatted("%04d_", captureCounter) + tag + ".bin";
    auto file = captureDir.getChildFile(fileName);
    file.replaceWithData(data.data(), (int) data.size());

    statusLabel.setText("Captured " + juce::String((int) data.size()) + " bytes -> " + fileName
                             + (isEditBuffer ? "  (edit buffer)" : "  (program " + juce::String(bank) + "/" + juce::String(program) + ")"),
                         juce::dontSendNotification);
}

void SysexCapturePanel::paint(juce::Graphics& g)
{
    g.setColour(LookAndFeelP12::panelBg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
}

void SysexCapturePanel::resized()
{
    auto b = getLocalBounds().reduced(8);
    title.setBounds(b.removeFromTop(18));
    b.removeFromTop(4);
    auto row = b.removeFromTop(26);
    captureButton.setBounds(row.removeFromRight(170));
    row.removeFromRight(6);
    tagBox.setBounds(row);
    b.removeFromTop(4);
    statusLabel.setBounds(b.removeFromTop(18));
}

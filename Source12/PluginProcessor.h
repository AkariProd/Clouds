#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "NrpnDecoder.h"
#include "MidiActivityLog.h"
#include <unordered_map>

// Seer12 is an INSTRUMENT: load it on an FL Studio instrument track, play/
// automate it, and it forwards notes + NRPN-encoded parameter changes to the
// Prophet 12 over MIDI out, while also decoding the P12's own transmitted
// NRPN stream to keep every on-screen knob in sync with the hardware panel.
// Fully bidirectional -- turning a knob here sends NRPN to the synth, and
// turning a knob on the synth updates the display here.
class Seer12Processor : public juce::AudioProcessor,
                         private juce::AudioProcessorValueTreeState::Listener
{
public:
    Seer12Processor();
    ~Seer12Processor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Bumped every time a value changes via incoming MIDI, so the editor can
    // tell "host/GUI edit" apart from "hardware knob move" if it ever needs
    // to (e.g. a future "last touched from hardware" indicator).
    std::atomic<int> lastNrpnParamIndex { -1 };

    // Diagnostic MIDI activity log, read by the editor's monitor panel to
    // show whether MIDI is reaching the plugin at all, and what the NRPN
    // decoder made of it -- see Source12/MidiMonitorPanel.
    p12::MidiActivityLog midiLog;

    // MIDI channel used both for outgoing NRPN and for notes forwarded to
    // the hardware. 1-16. TODO: expose in the UI; the Prophet 12 defaults
    // to channel 1 (Global > 1. MIDI Channel) unless changed.
    std::atomic<int> midiChannel { 1 };

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void sendNrpnOut(juce::MidiBuffer& out, int paramIndex, int rawValue, int samplePosition);

    p12::NrpnDecoder nrpnDecoder;
    std::unordered_map<std::string, int> idToIndex;

    // Set while the processor itself is writing a parameter value that it
    // just decoded FROM incoming hardware MIDI, so the parameterChanged
    // listener below doesn't immediately echo it straight back out as an
    // outgoing NRPN message.
    std::atomic<bool> suppressOutgoing { false };

    // Lock-free "a GUI/automation edit happened, please transmit it" queue:
    // one slot per parameter, written from parameterChanged (whatever thread
    // that fires on), drained once per audio block in processBlock.
    std::array<std::atomic<int>, p12::kNumParams> pendingOutRaw;
    std::array<std::atomic<bool>, p12::kNumParams> pendingFlag;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Seer12Processor)
};

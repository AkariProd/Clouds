#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "NrpnDecoder.h"
#include "MidiActivityLog.h"
#include "SysexCapture.h"
#include "DirectMidiLink.h"
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

    // Byte-layout reverse-engineering tool (see Source12/CONTEXT.md, "SysEx
    // dump capture"): call this from the UI to ask the hardware for a full
    // Program Edit Buffer Dump; the result shows up in sysexLog once it
    // arrives (polled from the editor's Timer, same pattern as midiLog).
    void requestEditBufferDump();
    p12::SysexCaptureLog sysexLog;

    // Direct, host-bypassing connection to the Prophet 12's real MIDI
    // ports, used only for the full-dump request/reply (see
    // DirectMidiLink.h for why: most VST3 hosts, FL Studio included,
    // don't pass SysEx through to/from a hosted plugin at all). Exposed
    // so the Settings UI can show connection status and offer Reconnect.
    p12::DirectMidiLink directLink;

    // Top-bar preset prev/next: sends a Program Change and immediately
    // follows it with a dump request (the hardware doesn't echo back a
    // Program Change it received FROM us, so the usual auto-sync-on-
    // incoming-PC path in processBlock never fires for our own sends).
    void changeProgram(int delta);
    std::atomic<int> currentProgramGuess { 0 };  // 0-127, best-effort local tracking only

    // Queues every one of the 245 params for outgoing NRPN, regardless of
    // whether its value actually changed -- used after loading a saved
    // Seer12 preset (DAW state recall, see setStateInformation) so the real
    // Prophet 12 ends up matching exactly, not just the params that happen
    // to differ from whatever was on the hardware before. Also reachable
    // from the UI as a manual "Push to hardware" action. The actual sends
    // are paced in processBlock (a few params per block, not all 245 at
    // once) so a slower MIDI interface/hardware input buffer doesn't drop
    // messages from one huge burst.
    void forceSendAllParams();

    // Full-preset sync: every ParamDef with a known dumpByteOffset (see
    // Source12/tools/byte_offset_map.json, reverse-engineered against real
    // hardware) gets set from the dump's raw bytes whenever one arrives --
    // either because the user clicked "Sync Now" (requestEditBufferDump)
    // or because an incoming Program Change was auto-detected. This is
    // what actually answers "every knob should update when I change
    // presets on the hardware".
    std::atomic<int> fullSyncSequence { 0 };       // bumped after each applied dump
    std::atomic<int> fullSyncAppliedCount { 0 };   // how many params the last sync touched
    juce::String getLastSyncedPatchName() const;

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void sendNrpnOut(juce::MidiBuffer& out, int paramIndex, int rawValue, int samplePosition);
    void applyFullDump(const std::vector<uint8_t>& dump);

    p12::NrpnDecoder nrpnDecoder;
    std::unordered_map<std::string, int> idToIndex;

    mutable juce::CriticalSection patchNameLock;
    juce::String lastSyncedPatchName;

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

    // Set by requestEditBufferDump() (called from the message thread, e.g. a
    // button click) and drained once per audio block in processBlock, which
    // actually queues the outgoing sysex request.
    std::atomic<bool> pendingDumpRequest { false };

    // Set by changeProgram() (message thread); drained in processBlock to
    // queue an outgoing Program Change. -1 means nothing pending.
    std::atomic<int> pendingProgramChange { -1 };

    // Round-robin scan position for the paced NRPN drain in processBlock
    // (see forceSendAllParams()) -- keeps a single live knob touch getting
    // sent immediately (it's found within a few scan steps either way)
    // while a 245-param forced push spreads itself over many blocks.
    std::atomic<int> paramDrainCursor { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Seer12Processor)
};

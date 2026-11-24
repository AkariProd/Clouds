#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "SettingsPanel.h"
#include "LookAndFeelP12.h"
#include <array>
#include <optional>
#include <unordered_map>

// Seer12 editor: the whole UI is a web page (Source12/ui/) shown in a JUCE 8
// WebBrowserComponent (WebView2 on Windows). The page is compiled into the
// plugin as binary data (target Seer12UI), so nothing is loaded from disk at
// runtime -- unless the SEER12_UI_DIR environment variable points at the ui/
// folder, in which case files are read live from there (edit + Ctrl+R in the
// plugin window to reload, no rebuild needed).
//
// Parameter sync (see ui/bridge.js for the JS side):
//   JS  -> C++  "seer:ready"   {}             -> we reply with a full snapshot
//               "seer:set"     {id, value}    -> param set (real, unnormalised value)
//               "seer:gesture" {id, begin}    -> begin/endChangeGesture for automation
//               "seer:program" {delta}        -> processor.changeProgram(delta)
//               "seer:sync"    {}             -> processor.requestEditBufferDump() (Sync Now)
//               "seer:settings" {}            -> toggle the native SettingsPanel drawer
//               "seer:presetList" {}          -> reply with the native preset list
//               "seer:presetLoad" {name}      -> load a native Seer12 preset
//               "seer:presetStep" {delta}     -> load prev/next native preset alphabetically
//               "seer:presetSave" {name}      -> save current state as a named native preset
//               "seer:presetDelete" {name}    -> delete a native preset
//   C++ -> JS   "seer:params"  {id: value...} -> changed values, polled at 30 Hz on the
//                                               message thread (so hardware knob moves
//                                               decoded by the processor show up live)
//               "seer:status"  {patch, link, synced, seq} -> header preset strip
//               "seer:presetList" {names, current} -> native preset list, after any change
//
// The settings drawer (Sync Now details, MIDI link reconnect, MIDI monitor,
// SysEx capture) stays a native JUCE component: it is shown BELOW the web view
// (the web view shrinks), since native components can't draw over a WebView.
class Seer12Editor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit Seer12Editor(Seer12Processor&);
    ~Seer12Editor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    static constexpr int kDesignWidth = 1440;
    static constexpr int kDesignHeight = 880;

private:
    void timerCallback() override;
    void pushValues(bool fullSnapshot);
    void pushStatus(bool force);
    void pushPresetList();
    void handleSet(const juce::var& payload);
    void handleGesture(const juce::var& payload);
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);
    juce::WebBrowserComponent::Options makeBrowserOptions();
    juce::RangedAudioParameter* paramFor(const juce::var& idVar) const;

    Seer12Processor& processor;

    // Declared before webView: the browser options capture `this` and these
    // must exist before any event can arrive.
    std::array<juce::RangedAudioParameter*, p12::kNumParams> params {};
    std::array<int, p12::kNumParams> lastSent {};
    std::unordered_map<juce::String, int> indexById;
    bool uiReady = false;
    int lastStatusSeq = -1;
    juce::String lastLinkText;

    LookAndFeelP12 lookAndFeel;      // for the native settings drawer
    SettingsPanel settingsPanel;
    bool settingsOpen = false;

    juce::WebBrowserComponent webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Seer12Editor)
};

# Seer12 — Context / Handoff Doc

Read this first if you're picking this project back up in a new chat.

## What this is

**Seer12** is a JUCE VST3/AU/Standalone plugin that acts as a live, on-screen
control-surface editor for the user's **Prophet 12** (Sequential/Dave Smith
Instruments hardware synth) — every knob on screen mirrors the hardware
panel in real time, in both directions.

- Not a Sequential/DSI product, no affiliation, no vendored firmware.
- Pure MIDI (NRPN) — **no audio DSP at all**. The plugin is an *instrument*
  (so it sits on an FL Studio instrument track) but produces silence itself;
  the real sound source is the hardware synth it's controlling.
- Branding ("Seer12") is deliberately NOT "Prophet 12" — trademark/trade-dress
  reasons (see the `porting-hardware-dsp-to-vst` Hermes skill, section 0).
  Easy to rename later if the user wants.

## Repo / build

- Lives inside the existing `skyjad` JUCE multi-plugin repo (same repo as
  Pebble, Cloudius/Skýjað), **not** a separate repo.
- Path: `C:\Users\produ\dev\skyjad\Source12\` (all Seer12 source).
- Git branch: **`seer12-prophet-editor`** (branched off `beads-playground`).
- CMake target added to the repo's root `CMakeLists.txt` (search for
  `Seer12` — it's a self-contained block near the end, mirrors the existing
  `Pebble` target's conventions but with no DSP sources).
- Build (from `C:\Users\produ\dev\skyjad\build`, already configured once):
  ```
  cmake --build . --target Seer12_Standalone --config Debug
  cmake --build . --target Seer12_VST3 --config Release
  ```
  VST3 auto-installs to `C:\Program Files\Common Files\VST3\Seer12.vst3`.
  A Start Menu shortcut named **Seer12** points at the Debug standalone exe
  (`Source12\...\Seer12_artefacts\Debug\Standalone\Seer12.exe`).
- JUCE checkout used: `C:\Users\produ\dev\JUCE` (pass
  `-DJUCE_DIR=C:/Users/produ/dev/JUCE` if reconfiguring from scratch).
- Generator: `cmake -G "Visual Studio 17 2022" -A x64 ..` (plain `cmake .`
  without explicit generator/arch has previously failed to detect the C++
  compiler in this environment — always pass both flags explicitly).

## How the parameter table was built

The Prophet 12 Module Operation Manual's "MIDI Implementation" chapter
publishes the full NRPN program-parameter table (246 real-time parameters
used here, out of 268 raw table rows — program-name characters and the
Arp Note/Velocity per-step arrays are excluded, no physical-knob equivalent).

- Manual PDF: `https://www.sequential.com/wp-content/uploads/2021/02/Prophet_12_Module-Op_Man_v1.2.pdf`
- Extraction pipeline lives in `Source12/tools/`:
  `extract3.py` (pdfplumber `page.extract_text()` + line regex — this is the
  reliable method; naive `pdftotext` column-zipping silently misaligns
  numbers/names, see the skill note below) → `build_params.py` (classifies
  each row into one of 26 sections, splits out enum option lists) →
  `gen_cpp.py` (code-generates `Parameters.h`/`Parameters.cpp` — don't hand-
  edit those, regenerate instead if the table needs to change).
- A detailed writeup of the extraction pitfalls is saved in the
  `porting-hardware-dsp-to-vst` Hermes skill (section 4b) for reuse on
  future "parse a synth manual's MIDI table" tasks.

## Architecture

- `PluginProcessor` — `juce::AudioProcessor`, **IS_SYNTH / instrument**
  (stereo silent output bus, no audio input). Holds a 246-parameter
  `AudioProcessorValueTreeState` (generated layout from `Parameters.cpp`).
- `NrpnDecoder` (`NrpnDecoder.h`) — decodes the P12's running-status NRPN
  stream: CC99/98 (param number MSB/LSB) then CC6 (+ optional CC38 for
  params needing >127 steps) → resolves to a parameter index + raw value.
- **Bidirectional now** (this was the big fix in this session — originally
  built as a receive-only MIDI *effect*, which is almost certainly why MIDI
  wasn't working: FL Studio routes "MIDI Effect" plugins very differently
  from instruments):
  - Incoming notes/pitch-bend/pressure are passed straight through to MIDI
    out, so a piano-roll pattern on this track plays the Prophet 12.
  - Turning a knob in the plugin (by hand or automation) queues an outgoing
    NRPN message (`sendNrpnOut`), sent on the next `processBlock`, so
    automating the plugin's filter etc. really moves the hardware.
  - A `suppressOutgoing` atomic flag stops a value just decoded FROM
    incoming hardware MIDI from immediately being echoed back out as if it
    were a new GUI edit (feedback-loop guard).
  - MIDI channel is currently hardcoded to 1 (`Seer12Processor::midiChannel`)
    — not yet exposed in the UI. TODO if the P12 isn't on channel 1.
- `MidiActivityLog` + `MidiMonitorPanel` — an always-visible diagnostic strip
  at the top of the editor: shows total CC received / NRPN resolved / NRPN
  sent, a blinking activity LED, and a scrolling raw log of the last ~16
  messages (`CH1  CC 99 =  0   -> Osc 1 Pitch = 120`-style). **Use this
  first** whenever MIDI "isn't working" — it immediately shows whether
  traffic is flowing in either direction before guessing at causes.
- GUI: `SectionPanel` (titled box + wrapped knob grid) for single-instance
  sections (Filters, Character, Feedback, Amp/Mix, Unison, Arp, Global...),
  plus `MultiInstanceSection` for every section that has 2+ numbered
  instances of the same thing (Oscillators, LFOs, Delays, Envelope 3/4, Mod
  Matrix — see below). Both implement `PanelLayoutable` so
  `Seer12PanelContent`'s flow-wrap layout (scrolled via `Viewport`) treats
  them identically. `ParamControl` (one knob/toggle/combo + label bound 1:1
  to an APVTS parameter via a normal Attachment, data-driven from
  `ParamDef`) is used for every single-instance section's controls;
  `LookAndFeelP12` is the shared original flat-cap knob skin, dark panel
  with red section headers — not a copy of Sequential's own editor UI.
- `MultiInstanceSection` + `MultiInstanceParamControl`: the Prophet 12's own
  panel reuses ONE row of controls across several numbered instances via a
  SELECT rather than giving each its own full knob row (OSC 1-4, LFO 1-4,
  Delay 1-4, Envelope 3/4, Mod Matrix slots 1-16) — Seer12 mirrors that.
  Field grouping is derived AUTOMATICALLY from the generated parameter names
  (`MultiInstanceSection`'s constructor searches for `"<prefix> <N> "`
  anywhere in each name, not just as a strict prefix — needed for names like
  "Velocity to Env 3 Amt" where the instance number isn't at the start).
  `MultiInstanceParamControl` is NOT a normal APVTS Attachment; turning it
  manually calls `setValueNotifyingHost()` on every *currently selected*
  instance's own independent parameter with the same value (so OSC 1 + OSC 3
  selected + turn "Pitch" retunes both in one move), and displays whichever
  selected instance has the lowest index. This still goes through the normal
  `parameterChanged` → outgoing-NRPN pipeline per affected parameter, so
  hardware sync keeps working unchanged.
- `CategorizedCombo`: a tree-style dropdown (a button that pops a
  `juce::PopupMenu` with per-category submenus) used for the Mod
  Source/Destination and LFO/Envelope Destination fields, which have 28-99
  options each (`ParamDef::treeCategorized == true`, option strings +
  per-option categories generated into `kOptionStrings`/`kOptionCategories`
  from `Source12/tools/mod_tables.py`, transcribed from the manual's
  Appendix A/B — those fields are plain knobs in the main NRPN table, no
  inline enum, so they needed a hand-built option list). NOT a
  `juce::ComboBox`/`ComboBoxAttachment` (JUCE's ComboBox can't do submenus);
  wired by hand with a `juce::Timer` poll, same pattern as
  `MultiInstanceParamControl`, in both `ParamControl` and
  `MultiInstanceParamControl` (Mod Matrix's shared Source/Destination knob
  goes through `MultiInstanceParamControl`'s treeCombo path too).

## Status as of this session

- ✅ Builds clean (Debug Standalone + Release VST3), both launch without
  crashing (verified via screenshot).
- ✅ Converted from MIDI-effect (receive-only) to full instrument
  (bidirectional) per user request.
- ✅ Oscillator section redesigned to one shared multi-select panel
  (OSC 1-4 buttons + one set of knobs), per user request — screenshot- and
  user-confirmed working (multi-select highlights correctly).
- ✅ Same multi-select pattern generalized to LFOs, Delays, Envelope 3/4,
  and the 16-slot Mod Matrix — screenshot-confirmed rendering correctly.
  Clicking a tree dropdown (Mod Destination etc.) wasn't confirmed via
  automated screenshot (PopupMenu doesn't show reliably under synthetic
  input) — should be eyeballed once by hand.
- ✅ Mod Source/Destination + LFO/Env Destination are now tree-categorized
  dropdowns instead of raw 0-98 knobs, per user request.
- ❌ **Still not yet verified against the real hardware.** User reported
  "MIDI isn't working" while it was still the effect version; the instrument
  conversion is the fix attempt but hasn't been tested with the P12
  physically connected yet.
- User will test next time they're at the studio with the Prophet 12
  connected. **Next step: load Seer12 on an FL Studio instrument track,
  route its MIDI out to the P12, confirm GLOBAL > 7 is set to NRPN on the
  hardware, and watch the MidiMonitorPanel strip for activity in both
  directions.**

## Known limitations / TODO

- MIDI channel hardcoded to 1, not exposed in UI yet.
- Program name characters (20 NRPNs) and Arp Note/Velocity step arrays
  (66 NRPNs) are real parameters but have no panel UI (no physical knob
  equivalent on the hardware either).
- Layer A vs Layer B (the P12 is dual-layer): both NRPN ranges map to the
  same on-screen knob; outgoing NRPN always transmits on the Layer A
  number. Fine for single-layer use; may need work for split/stack patches.
- No automated tests; verification so far is "launch + screenshot +
  eyeball". If something still doesn't sync once tested against real
  hardware, the MidiMonitorPanel log is the first debugging tool to reach
  for.

## Update this file

Whenever you make meaningful progress or discover something (e.g. "tested
against real hardware, X worked / Y didn't because Z"), update the
**Status** and **Known limitations** sections above before ending the
session, so the next chat picks up from an accurate state instead of
re-discovering the same ground.

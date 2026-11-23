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
- ✅ **Verified live against real hardware** (first time this got actually
  tested with the physical P12 connected). Found and fixed real bugs along
  the way:
  - NRPN decoder was completely broken for narrow-range params (always
    showed 0% or 100%) -- fixed, see NrpnDecoder.h: always wait for both
    CC6+CC38 and combine as 14-bit, never trust CC6 alone.
  - FL Studio plugin-type cache (Effect vs Instrument) needs manually
    deleting `Documents\Image-Line\FL Studio\Presets\Plugin
    database\Installed\Effects\VST3\Seer12.*` after any IS_SYNTH change,
    FL won't auto-detect the category switch from a rescan alone.
- ✅ **Full preset sync built and verified** (the actual original ask --
  "every knob should have the correct value when I change presets"):
  - `Source12/tools/auto_map_bytes.py` fully automates reverse-engineering
    the undocumented SysEx Program/Edit Buffer Dump byte layout -- talks
    directly to the hardware via raw `python-rtmidi` (no GUI involved),
    sets each param via NRPN, requests a dump, diffs it against the
    previous one. Mapped 245/245 parameters in under a minute.
    `byte_offset_map.json` holds the result; `gen_cpp.py` bakes it
    straight into each `ParamDef.dumpByteOffset`.
  - `Seer12Processor::applyFullDump()` + the new `HardwareSyncPanel`
    ("Sync Now" button) actually consume that map: one click (or an
    auto-detected incoming Program Change) pulls a dump and sets all 245
    on-screen controls at once, plus reads the patch name from bytes
    402-421. **Confirmed working end-to-end against the real P12**: "Sync
    Now" correctly synced 245 params and showed the real loaded patch
    name ("NUm!erONE").
  - This reverse-engineering also caught two real bugs baked into the
    manual's own NRPN table (see git log on this branch for the
    `build_params.py` override fixes): BPM and A/B Mode printed the same
    NRPN number (288 really is A/B Mode; BPM dropped until its real
    number is found, rather than ship a knob that silently corrupts A/B
    Mode); and Delay N Pan's printed numbers (427-430) were off by two
    (real numbers are 429-432).

## Known limitations / TODO

- MIDI channel hardcoded to 1, not exposed in UI yet.
- Program name characters (20 NRPNs, not individually controllable -- the
  decoded name is shown read-only via HardwareSyncPanel instead) and Arp
  Note/Velocity step arrays (66 NRPNs) have no panel UI (no physical knob
  equivalent on the hardware either).
- Layer A vs Layer B (the P12 is dual-layer): both NRPN ranges map to the
  same on-screen knob; outgoing NRPN always transmits on the Layer A
  number. Fine for single-layer use; may need work for split/stack patches.
- BPM has no working control at all right now -- its real NRPN number
  was never found (see "Full preset sync" above). A brute-force scan of
  nearby unclaimed numbers (296-300) didn't turn up a clean match either.
  Low priority; revisit if the user wants global tempo control.
- Delay 1/2 Pan vs NRPN 427/428 genuinely do nothing (confirmed, not a bug
  on our end) -- only 429-432 are real, already fixed and in use.
- No automated tests; verification is "launch + screenshot + eyeball",
  now backed by actual hardware testing (both NRPN round-trip and the
  full dump sync are confirmed live, not just assumed from code reading).

## Update this file

Whenever you make meaningful progress or discover something (e.g. "tested
against real hardware, X worked / Y didn't because Z"), update the
**Status** and **Known limitations** sections above before ending the
session, so the next chat picks up from an accurate state instead of
re-discovering the same ground.

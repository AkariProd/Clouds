# Seer12 — Project Handoff / Onboarding Doc

**Read this file FIRST, in full, before touching any code.** It's written so
an AI (or human) with zero prior context can pick this project up cold and
start modifying it correctly on the first try.

---

## 1. What this project is (one paragraph)

Seer12 is a custom JUCE audio plugin (VST3 + AU + Standalone) that works as
a live, bidirectional, on-screen control-surface editor for a **Sequential/
Dave Smith Instruments Prophet 12** hardware synthesizer. Every one of the
Prophet 12's ~245 real-time panel parameters (oscillators, LFOs, envelopes,
delays, filter, mod matrix, arpeggiator, etc.) has an on-screen knob/dropdown
in Seer12. Turning a knob in Seer12 sends MIDI to the real hardware and
changes the real sound; turning a knob on the real hardware updates the
on-screen control in Seer12. It also plays the hardware like a normal VST
instrument (notes from a piano-roll/MIDI track reach the P12 and it makes
sound), and can pull a full snapshot of whatever patch is currently loaded
on the hardware so every knob is correct after a preset change — not just
the one you touched.

**It is not a Sequential/DSI product.** No vendored firmware, no copied UI
graphics, no use of the "Prophet 12" name/logo in the plugin's own branding
— this is an original clean-room implementation built entirely from the
manufacturer's published MIDI Implementation chapter plus hardware-verified
reverse engineering (see §5). Legally and technically independent.

**The plugin makes no sound of its own.** It's registered as an instrument
(`IS_SYNTH`, stereo output bus) so DAWs will host it on an instrument track,
but `processBlock` only ever writes silence to the audio buffer — the real
instrument is the hardware synth it's puppeting over MIDI.

---

## 2. Quick start: build, run, and see it working

```
cd C:\Users\produ\dev\skyjad\build
cmake -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR="C:/Users/produ/dev/JUCE" ..
cmake --build . --target Seer12_Standalone --config Debug
cmake --build . --target Seer12_VST3 --config Release
```
- Plain `cmake .` **without** explicit `-G`/`-A` has previously failed to
  detect the compiler in this environment — always pass both flags.
- `Seer12_Standalone` → `build\Seer12_artefacts\Debug\Standalone\Seer12.exe`
  (also has a Start Menu shortcut called "Seer12"). Standalone owns real
  hardware MIDI ports directly (via JUCE's own device picker/menu) — this
  is the fastest way to test a change without touching FL Studio at all.
- `Seer12_VST3` Release auto-installs to
  `C:\Program Files\Common Files\VST3\Seer12.vst3` on build. Load it in FL
  Studio on an instrument channel-rack slot (reload the plugin — or
  restart FL — to pick up a fresh build; FL caches loaded DLLs).
- Both targets must be rebuilt after any change to get a change to show up
  — there's no hot-reload.
- After a build, **kill any running `Seer12.exe`/FL Studio process first**
  if the build fails to overwrite the binary (Windows locks EXE/DLL files
  that are running): `taskkill /F /IM Seer12.exe` (and close FL Studio, or
  reload the plugin to release its handle on the VST3 DLL).

To verify a change actually works, don't just eyeball the code — launch
Standalone, screenshot it (this repo has been developed almost entirely via
an AI driving a real GUI through `computer_use`/accessibility APIs and
reading back screenshots), and ideally test against the real hardware if
it's connected.

---

## 3. Hardware setup this was developed against

- Real Prophet 12 connected via USB, MIDI port name **"Prophet 12
  Keyboard"** (shows up as both an input and output device under that
  exact name in Windows).
- Hardware MIDI channel: 1 (the P12 default; Global > 1. MIDI Channel).
- Hardware NRPN must be enabled (it is, by Sequential default) for live
  knob sync to work at all.
- For the auto-sync-on-preset-change feature, the hardware's
  **GLOBAL > MIDI Program Change Send** must be set to On, or the P12
  never tells anyone it changed patches.

---

## 4. Repo / file map

Lives inside an existing multi-plugin JUCE repo, **not its own repo** —
don't be surprised to see unrelated plugins (Pebble, Cloudius/Skyjad) in
the same tree; they are separate products, ignore them.

- Repo root: `C:\Users\produ\dev\skyjad` (git remote `origin` =
  `https://github.com/AkariProd/Clouds.git`)
- **Git branch: `seer12-prophet-editor`** — all Seer12 work happens here,
  branched off `beads-playground`. Don't touch `Kubika_05` (off-limits,
  unrelated project) or `main` (has one unrelated commit from fixing a
  different plugin's filename bug — don't merge Seer12 work into it).
- All Seer12 source: `Source12\` — CMake target block is in the repo root
  `CMakeLists.txt` (search for `Seer12`).

### File-by-file map

| File | What it is |
|---|---|
| `PluginProcessor.h/.cpp` | The `juce::AudioProcessor`. Owns the 245-param `AudioProcessorValueTreeState`, the NRPN decoder, the direct hardware MIDI link, full-dump apply logic, Program-Change auto-trigger. Start here. |
| `Parameters.h/.cpp` | **Generated, do not hand-edit.** The 245-entry `kParams[]` table (id, name, section, NRPN numbers, min/max, type, dump byte offset...). Regenerate via `tools/gen_cpp.py` instead. |
| `NrpnDecoder.h` | Decodes the P12's NRPN MIDI stream (CC99/98 = param number, CC6+CC38 = 14-bit value) into a parameter index + raw value. |
| `DirectMidiLink.h` | **Important, non-obvious.** Opens a direct connection to the Prophet 12's real MIDI ports, bypassing whatever the host (FL Studio etc.) does with the plugin's own MIDI buses. Exists because VST3 hosts generally don't pass SysEx through to/from a hosted plugin — see §6. Used only for the full-dump sync feature. |
| `SysexCapture.h` | SysEx message builders/parsers for the Prophet 12's "Request Program Edit Buffer Dump" protocol, plus the packed-MSB unpacker (manual Appendix D format). |
| `MidiActivityLog.h` / `MidiMonitorPanel.h/.cpp` | Diagnostic MIDI traffic strip (CC in/out counters + scrolling raw log). First thing to check whenever "MIDI isn't working." |
| `HardwareSyncPanel.h/.cpp` | "Sync Now" button + hardware-link connection status + Reconnect button. Pulls a full preset dump on demand. |
| `SysexCapturePanel.h/.cpp` | Manual SysEx-dump capture tool (tag + capture button) — a dev tool used during the original byte-layout reverse-engineering project, kept around in case the byte map ever needs re-verifying. |
| `SettingsPanel.h/.cpp` | Collapsible drawer bundling the three dev/debug tools above (Sync Now, MIDI monitor, SysEx capture) — hidden by default, toggled from the top bar's gear icon, so the main view stays clean for actual playing/editing. |
| `TopBar.h/.cpp` | Header bar: title, a preset-name "LCD well" (shows the last-synced patch name), prev/next buttons (send real Program Change to the hardware + auto re-sync), and the settings gear. |
| `ParamControl.h/.cpp` | One knob/toggle/combo + label, bound 1:1 to an APVTS parameter. Used for every single-instance section. |
| `MultiInstanceSection.h/.cpp`, `MultiInstanceParamControl.h/.cpp` | For sections where the hardware itself reuses one row of controls across several numbered instances via a SELECT (OSC 1-4, LFO 1-4, Delay 1-4, Envelope 3/4, Mod Matrix slots 1-16) — see §7.1. |
| `CategorizedCombo.h/.cpp` | Tree-style dropdown (submenu popup) for the Mod Source/Destination and LFO/Env Destination fields, which have 28-99 options each with manual category groupings — plain `juce::ComboBox` can't do submenus. |
| `SectionPanel.h/.cpp` | Titled box + wrapped knob grid, the layout container for single-instance sections. |
| `LookAndFeelP12.h/.cpp` | Shared visual skin — original flat-cap knob design, dark panel, colour constants (`panelBg`, `headerBg`, `accent`, etc). Not a copy of Sequential's own editor graphics. |
| `PluginEditor.h/.cpp` | Top-level editor Component: lays out `TopBar`, `SettingsPanel` (when open), and the scrollable `Seer12PanelContent` holding every section. |
| `tools/build_params.py`, `tools/gen_cpp.py`, `tools/extract3.py` | The parameter-table generation pipeline — see §5. |
| `tools/mod_tables.py` | Hand-transcribed option lists + category groupings for Mod Source/Destination and LFO/Delay Sync Setting dropdowns (these aren't in the manual's main NRPN table, needed separate transcription). |
| `tools/auto_map_bytes.py` | Standalone Python script that reverse-engineered the full SysEx dump byte layout against real hardware. Not part of the plugin build; a one-time research tool, kept for re-running if the byte map ever needs re-verification. |
| `tools/byte_offset_map.json` | The 245-entry result of that reverse-engineering: `{ param_id: { byte, mirror_byte } }`. Consumed by `gen_cpp.py` to bake `dumpByteOffset` into `Parameters.cpp`. |

---

## 5. How the 245-parameter table was built (and how to change it)

The Prophet 12 Module Operation Manual's "MIDI Implementation" chapter
publishes the NRPN program-parameter table (268 raw rows; 245 used here —
program-name characters and Arp Note/Velocity per-step arrays are excluded,
no physical-knob equivalent on the hardware).

- Manual: `https://www.sequential.com/wp-content/uploads/2021/02/Prophet_12_Module-Op_Man_v1.2.pdf`
- Pipeline: `extract3.py` (pdfplumber `page.extract_text()` + line regex —
  **this is the reliable method**; naive `pdftotext` column-zipping
  silently misaligns numbers/names) → `build_params.py` (classifies each
  row into one of 26 sections, splits out enum option lists, applies
  hand-written override tables for things the manual's auto-extraction
  gets wrong — see below) → `gen_cpp.py` (code-generates
  `Parameters.h/.cpp`, reading `tools/byte_offset_map.json` too for the
  full-dump-sync byte offsets).
- **To change anything about a parameter** (name, range, section, NRPN
  number, enum options): edit `build_params.py`'s override tables or
  source data, never hand-edit `Parameters.h/.cpp` directly — your edit
  will be silently lost the next time someone regenerates. Then re-run
  the pipeline and rebuild.
- A few things the manual's printed table got **wrong**, discovered via
  real-hardware testing and now corrected in `build_params.py`:
  - BPM and A/B Mode both printed NRPN 288 in the manual. 288 really
    belongs to A/B Mode. BPM has no confirmed real NRPN number and was
    **removed** from the parameter set entirely (246 → 245 total).
  - Delay 1/2/3/4 Pan's printed NRPN numbers (427-430) are off by two —
    the real numbers are **429/430/431/432** (confirmed: 427/428 produce
    no hardware response at all).
  - LFO Sync Setting (16 options) and Delay Sync Setting (12 options)
    option lists were truncated by a PDF-page-break bug in the original
    auto-extraction (down to 3 and 5 options respectively) and hand-fixed
    by transcribing the full lists from manual pages 81-84 v1.2.

---

## 6. The two MIDI protocols in play, and the critical VST3/SysEx gotcha

Seer12 talks to the hardware two completely different ways, and **mixing
up which one is used where is the single most important thing to
understand about this codebase**:

### 6.1 NRPN (plain MIDI CC) — for live, per-knob sync

- Outgoing (plugin → hardware): CC99/98 (param number MSB/LSB), then
  CC6/CC38 (14-bit value MSB/LSB) — see `sendNrpnOut` in
  `PluginProcessor.cpp`. Always sends both value bytes, even for narrow
  0-19-range params where CC6 is always 0 — the hardware does the same,
  and an earlier bug (see git log) that tried to skip CC6 for "narrow"
  params broke every such parameter.
- Incoming (hardware → plugin): same four CCs, decoded by
  `NrpnDecoder.h`, applied to the matching APVTS parameter.
- This travels through the **normal host-routed MIDI path**
  (`processBlock`'s `MidiBuffer` in/out) and works fine through any VST3
  host, FL Studio included — plain MIDI CC/Note/Program-Change messages
  are fully supported by the VST3 spec and every host.
- **FL-specific setup required for this to work at all**: FL Studio must
  have the Prophet 12's MIDI input device **enabled** in MIDI Settings
  (F10), and the Seer12 plugin wrapper's input port needs to match (or be
  left on the default/omni routing FL gives instrument tracks). If
  someone previously disabled that input device (e.g. to let a different
  tool grab the port exclusively, as happened during the original
  byte-mapping research), knob sync silently stops working with no error.

### 6.2 SysEx — for full-preset-dump sync, and why it needs a workaround

- The Prophet 12 supports a documented SysEx "Request Program Edit Buffer
  Dump" (`F0 01 2A 06 F7`), replying with a ~1024-byte packed snapshot of
  every parameter in the currently-loaded patch (see `SysexCapture.h`).
  This is what makes "every knob updates correctly when I change presets
  on the hardware" possible — NRPN alone only tells you about whatever
  you just touched.
- **The critical gotcha: VST3 as a format does not reliably pass SysEx
  through to or from a hosted plugin, and FL Studio specifically does
  not forward SysEx out of a hosted VST at all** (confirmed via
  Image-Line's own forum/support answers). This is not a bug in Seer12 —
  it's a host/format limitation that bit this project directly: the
  dump-request/reply mechanism worked perfectly in Standalone (which owns
  real hardware MIDI ports directly, no host SysEx filtering involved)
  but got permanently stuck on "Requesting..." the moment it was tested
  inside FL Studio.
- **The fix, already implemented**: `DirectMidiLink.h`. The plugin opens
  its own direct connection to the Prophet 12's real MIDI ports (matched
  by "prophet" substring in the device name, auto-connects on
  construction), completely independent of whatever the host is doing
  with the plugin's declared MIDI in/out buses. The dump request is sent
  and the reply is received entirely through this side-channel; only
  notes/CC/Program-Change still go through the normal host-routed path
  (which works fine, since those aren't SysEx).
  - `HardwareSyncPanel` shows this link's live connection status and has
    a manual "Reconnect" button for if auto-connect ever fails (e.g. a
    different MIDI port name on a different machine, or driver-level
    exclusive-access conflicts).
  - **If you ever add a new SysEx-based feature, route it through
    `DirectMidiLink`, not through `processBlock`'s host-provided
    `MidiBuffer`** — the latter will silently never reach the hardware
    when hosted in FL Studio (or most other VST3 hosts).

---

## 7. Architecture deep dive

- `PluginProcessor` — `juce::AudioProcessor`, instrument (stereo silent
  output bus, `acceptsMidi()`/`producesMidi()` both true). Holds the
  245-param `AudioProcessorValueTreeState`.
- Incoming notes/pitch-bend/pressure are passed straight through to MIDI
  out untouched, so a piano-roll pattern on this instrument track plays
  the Prophet 12 for real.
- Turning a knob in the plugin (GUI or host automation) queues an
  outgoing NRPN message via a lock-free per-parameter pending-flag array,
  drained once per `processBlock` call (`sendNrpnOut`).
- A `suppressOutgoing` atomic flag stops a value just decoded FROM
  incoming hardware MIDI from being immediately echoed back out as if it
  were a brand new GUI edit (feedback-loop guard).
- MIDI channel is currently hardcoded to 1 (`Seer12Processor::midiChannel`)
  — not yet exposed in the UI.

### 7.1 Multi-instance sections

The Prophet 12's own hardware panel reuses ONE row of physical knobs
across several numbered instances via a SELECT button (OSC 1-4, LFO 1-4,
Delay 1-4, Envelope 3/4, Mod Matrix slots 1-16) rather than giving every
instance its own permanent row — Seer12 mirrors that with
`MultiInstanceSection` + `MultiInstanceParamControl`:
- Field grouping is derived **automatically** from parameter names
  (`MultiInstanceSection`'s constructor searches for `"<prefix> <N> "`
  anywhere in each name, not just as a strict prefix — needed for names
  like "Velocity to Env 3 Amt" where the instance number isn't at the
  start).
- `MultiInstanceParamControl` is **not** a normal APVTS Attachment;
  turning it manually calls `setValueNotifyingHost()` on every currently
  *selected* instance's own independent parameter with the same value (so
  selecting OSC 1 + OSC 3 and turning "Pitch" retunes both in one move),
  displaying whichever selected instance has the lowest index. This still
  goes through the normal `parameterChanged` → outgoing-NRPN pipeline per
  affected parameter, so hardware sync is unaffected.

### 7.2 Tree-categorized dropdowns

`CategorizedCombo` — a button that pops a `juce::PopupMenu` with
per-category submenus — used for Mod Source/Destination and LFO/Envelope
Destination fields (28-99 options each, `ParamDef::treeCategorized ==
true`). These fields are plain knobs in the manual's NRPN table with no
inline enum, so their option lists + category groupings were hand-
transcribed into `tools/mod_tables.py` from the manual's Appendix A/B.
Not a `juce::ComboBox` (can't do submenus); wired by hand with a
`juce::Timer` poll, same pattern as `MultiInstanceParamControl`.

### 7.3 UI layout

`SectionPanel` (titled box + wrapped knob grid) for single-instance
sections (Filters, Character, Feedback, Amp/Mix, Unison, Arp, Global...);
`MultiInstanceSection` for the reused-row sections (§7.1). Both implement
`PanelLayoutable` so `Seer12PanelContent`'s flow-wrap layout (scrolled via
a `Viewport`) treats them identically. `TopBar` + `SettingsPanel` (§4)
sit above the scrollable content, outside the `Viewport`.

---

## 8. Known limitations / TODO

- MIDI channel hardcoded to 1, not exposed in UI yet.
- Program name characters (20 NRPNs, not individually controllable — the
  decoded name is shown read-only in the top bar instead) and Arp
  Note/Velocity step arrays (66 NRPNs) have no panel UI (no physical knob
  equivalent on the hardware either).
- Layer A vs Layer B (the P12 is dual-layer): both NRPN ranges map to the
  same on-screen knob; outgoing NRPN always transmits on the Layer A
  number. Fine for single-layer use; may need work for split/stack
  patches.
- BPM has no working control at all — its real NRPN number was never
  found (a brute-force scan of nearby unclaimed numbers 296-300 didn't
  turn up a clean match either). Low priority; revisit if the user wants
  global tempo control.
- No automated tests; verification is "launch + screenshot + eyeball",
  backed by live hardware testing for the MIDI-critical paths (NRPN
  round-trip and full-dump sync both confirmed against the real P12, not
  just assumed from code reading).
- Visual design is a first pass (flat-cap knobs, dark panel, red section
  headers, GForce/Arturia-style top preset bar) — functional and
  original, not yet a deep polish pass. Good next step if picking this
  back up for UI work: look at what sections feel cluttered/cramped at
  current window size, consider per-section colour accents, knob-cap
  shading/gradients for more depth.

---

## 9. How to extend this project (for a fresh AI starting from here)

1. **Read this whole file first.** Then skim `PluginProcessor.h` and
   `PluginEditor.h` to see the actual member layout.
2. **Adding a brand-new on-screen control for an existing parameter that's
   already in `Parameters.cpp`**: just add a `ParamControl` (or
   `MultiInstanceParamControl` if it's a reused-row section) to the
   relevant `SectionPanel`/`MultiInstanceSection` in `PluginEditor.cpp`'s
   `Seer12PanelContent` constructor — no processor changes needed, NRPN
   wiring is fully data-driven from `ParamDef`.
3. **Adding/changing a parameter's definition** (name, range, NRPN
   number, section): edit `tools/build_params.py`'s override logic, never
   `Parameters.h/.cpp` directly, then re-run the generation pipeline
   (`extract3.py` → `build_params.py` → `gen_cpp.py`) and rebuild both
   targets.
4. **Adding a new SysEx-based feature**: route it through
   `DirectMidiLink`, not the host-provided `MidiBuffer` — see §6.2. This
   is the single most important lesson of this project; skipping it will
   produce a feature that works perfectly in Standalone and silently does
   nothing in FL Studio, which is exactly the bug this doc exists to
   prevent you from re-introducing.
5. **Before declaring anything "done"**: rebuild both Debug Standalone
   and Release VST3, launch Standalone and verify visually (screenshot),
   and if a change touches MIDI behavior, test against the real hardware
   if at all possible — several serious bugs in this project's history
   (narrow-range NRPN decode, wrong Delay Pan NRPN numbers, the SysEx/VST3
   gotcha above) were invisible from reading the code and only surfaced
   under real hardware testing.
6. **Commit to the `seer12-prophet-editor` branch**, write a commit
   message that explains *why* (root cause, not just what changed) — this
   file plus good commit messages are the project's only persistent
   memory across chat sessions.
7. **Update this file** whenever you learn something that would have
   saved you time if you'd known it from the start (a gotcha, a wrong
   assumption, a host quirk). That's the whole point of this document.

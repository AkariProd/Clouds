# Seer12

An original live hardware-synth panel editor: a MIDI-only VST3/AU/Standalone
(no DSP, no audio) that mirrors a Prophet 12's full set of ~246 panel
parameters and keeps every knob in sync with the hardware over MIDI NRPN.

Not a Sequential/DSI product, not affiliated with them, no vendored firmware
or panel graphics -- the knob skin (LookAndFeelP12) and layout code here are
original. The parameter table (Source12/Parameters.cpp) is generated from the
numbers published in Sequential's own Prophet 12 Module Operation Manual
("MIDI Implementation" chapter, NRPN program-parameter table) -- see
tools/extract3.py / build_params.py / gen_cpp.py for the extraction +
codegen pipeline (re-run against manual.pdf if Sequential revises the table).

## Hardware setup required

On the Prophet 12: GLOBAL -> 7. MIDI Param Send -> set to **NRPN** (not CC).
NRPN covers the full parameter range; CC mode is 7-bit-only and would miss
several wide-range parameters. Route the P12's MIDI output into this
plugin's MIDI input track in FL Studio (same routing already used for
playing it, since this plugin needs no separate MIDI port).

## Known limitations (v1)

- Program name characters and the Arp Note/Velocity per-step arrays are
  real NRPN parameters but aren't exposed as panel knobs (no equivalent
  physical control on the synth either).
- Layer A vs Layer B: both NRPN ranges map to the same on-screen knob, so
  whichever layer the P12 is currently editing drives the display.
- One-way only: moving a knob in the plugin does not currently transmit
  back to the hardware (MIDI output not wired up yet).

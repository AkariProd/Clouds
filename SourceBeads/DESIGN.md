# DESIGN.md — UI Brief for the Granular Texture Plugin (Beads-style)

> **Read this before touching any UI code.** Every visual decision should trace back to this file.
> If something here conflicts with a quick fix, this file wins — or ask Orfeo.

---

## 1. The one-line brief

**A granular texture instrument that feels like Apple designed it: calm, precise, quiet until you touch it — with one living, beautiful grain visualizer at its heart.**

Three words to check every screen against: **Calm. Precise. Alive.**

- *Calm* — few colors, lots of space, nothing shouting.
- *Precise* — perfect alignment, crisp type, exact values always visible.
- *Alive* — the visualizer and subtle motion are the only "show"; everything else stays out of the way.

---

## 2. Identity & legal guardrails

- This is a **clone of the DSP behaviour** of Mutable Instruments Beads (MIT-licensed code). It is **not** a replica of the hardware panel.
- Do **not** use the names "Mutable Instruments" or "Beads", or the MI logo, panel artwork or panel colors, anywhere in the UI. Use our own product name (placeholder: `GRAIN` until decided).
- Reference plugins (section 9) are for **feel only**. Never copy their assets, layouts pixel-for-pixel, icons or color palettes.

---

## 3. Layout

Default size **880 × 560 px**, resizable with a locked aspect ratio between 75% and 200%. Every measurement below is at 100%.

```
┌──────────────────────────────────────────────────────────────────┐
│  HEADER  (48px)                                                  │
│  ● GRAIN    ‹ Preset name ›        [ Mode ▾ segmented ]    ⚙     │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  VISUALIZER  (≈ 240px — the hero, full width, inset 16px)        │
│  rolling input buffer + live grains                              │
│                                       [ FREEZE ]  [ SEED ]       │
├──────────────────────────────────────────────────────────────────┤
│  GRAIN TILES                              │  OUTPUT TILES         │
│ ┌──────┐┌──────┐┌──────┐┌──────┐┌──────┐  │ ┌──────┐┌──────┐┌──────┐│
│ │ TIME ││ SIZE ││SHAPE ││PITCH ││DENS. │  │ │FDBK  ││ WET  ││ VERB ││
│ │  ◯   ││  ◯   ││  ◯   ││  ◯   ││  ◯   │  │ │  ◯   ││  ◯   ││  ◯   ││
│ │ 0.42 ││ 85ms ││ 0.30 ││ +7st ││ rand ││  │ │ 35%  ││ 60%  ││ 20%  ││
│ └──────┘└──────┘└──────┘└──────┘└──────┘  │ └──────┘└──────┘└──────┘│
├──────────────────────────────────────────────────────────────────┤
│  FOOTER (28px)   Input ▮▮▮▯▯  gain        CPU · grains: 42        │
└──────────────────────────────────────────────────────────────────┘
```

**Rules**
- Use an **8 px grid** (4 px for fine adjustments). Every x, y, width and height is a multiple of 4.
- Outer margin is 16 px. The gap between tiles is 8 px. The gap between tile groups is 24 px, with an optional 1 px hairline divider.
- Groups are sorted by meaning: **Grain** (what grains are) on the left, **Output** (what happens to the sound) on the right.
- The **parameter list comes from the DSP code**: confirm the exact names, ranges and units against the processor. The layout above is the expected set (Time, Size, Shape, Pitch, Density, Feedback, Dry/Wet, Reverb, Freeze, Seed, Input gain, quality modes). Add or remove tiles to match the processor, but keep the grid.
- Modulation and attenuverter amounts do **not** get their own row of knobs. They show as a thin secondary arc on each tile's knob (see 5.2), which can be edited in a "Mod" view toggled from the header.

---

## 4. Design tokens

All tokens live in **one place**: `ui/Theme.h` (a `struct Theme` plus a custom `LookAndFeel_V4` subclass). **Never hardcode a color, size or font in `paint()`.**

### 4.1 Color — Dark theme (default)

| Token | Value | Use |
|---|---|---|
| `bg` | `#0E0F11` | Window background |
| `surface` | `#17181B` | Visualizer panel, header |
| `tile` | `#1E1F23` | Control tiles |
| `tileHover` | `#24262B` | Tile on hover |
| `hairline` | `rgba(255,255,255,0.06)` | Borders, dividers |
| `track` | `rgba(255,255,255,0.10)` | Knob/slider track |
| `textPrimary` | `#F2F2F5` | Values, titles |
| `textSecondary` | `#8E8E93` | Labels |
| `textTertiary` | `#5A5A60` | Units, disabled |
| `accent` | `#FFB547` (warm amber) | The **only** UI accent: active arcs, focus, Freeze on |
| `accentSoft` | `accent @ 16%` | Accent backgrounds (active button fill) |
| `danger` | `#FF5A52` | Clipping only |

**Grain colors**, used in the visualizer only: map grain pitch to a smooth gradient of
`#5AC8FA` (−24 st) → `#F2F2F5` (0 st) → `#FFB547` (+24 st). No other colors appear in the visualizer.

### 4.2 Color — Light theme (optional, phase 2)
`bg #F5F5F7`, `surface #FFFFFF`, `tile #FFFFFF` with a `0 1px 2px rgba(0,0,0,0.06)` shadow, `textPrimary #1D1D1F`, `textSecondary #6E6E73`, same accent darkened to `#E8961C`.

### 4.3 Typography
- Font: **Inter** (OFL, embed via BinaryData). Do not embed SF Pro, because its license does not allow it.
- Always turn on **tabular numbers** for values, so digits don't jitter while dragging.

| Token | Size / weight / tracking | Use |
|---|---|---|
| `title` | 15 / SemiBold / −0.2 | Product name, preset name |
| `label` | 10 / Medium / +0.8, UPPERCASE | Tile labels (TIME, SIZE…) |
| `value` | 13 / Medium / 0, tabular | Value readouts |
| `unit` | 10 / Regular, `textTertiary` | "ms", "st", "%" |
| `micro` | 9 / Regular | Footer, tooltips |

### 4.4 Shape & depth
- Radius: tiles **12 px**, visualizer panel **14 px**, buttons are **pills** (height/2), and the header has none.
- Borders: 1 px `hairline`, drawn **inside** the bounds (inset 0.5 px for crispness).
- Depth comes from **surface color steps**, not shadows. At most one very soft shadow per element. No bevels, no gloss, no textures, no skeuomorphism.

### 4.5 Motion
- Use the JUCE 8 animation module. State changes (hover, press, toggle): **150 ms, ease-out**.
- The value readout updates **instantly**. Only decorative things animate.
- Freeze toggle: a 250 ms crossfade of the visualizer into its frozen state (see 6.4).
- Never animate layout. Nothing moves position except grains.

---

## 5. Components

### 5.1 Control tile ("the little boxes")
The core building block. Each parameter gets one tile.

- Size: **88 × 112 px**. Fill `tile`, 1 px `hairline` border, radius 12.
- Content, centered vertically:
  1. `label` at the top (y = 12), `textSecondary`
  2. A knob in the middle (56 px diameter)
  3. `value` + `unit` at the bottom (y = bottom − 14)
- Hover: fill becomes `tileHover` and the label becomes `textPrimary`.
- Drag: the border becomes `accent @ 40%`.
- Focus (keyboard): 2 px `accent` ring outside the tile, offset 2 px.
- The whole tile is the hit area, not just the knob.

### 5.2 Knob
- Flat. **A 270° arc**, gap at the bottom.
- Track: 3 px stroke, `track` color, round caps.
- Value arc: 3 px stroke in `accent`. **Unipolar** parameters fill from the minimum. **Bipolar** parameters (Pitch, and others centered on a neutral value) fill **from 12 o'clock** outwards.
- Indicator: a 4 px dot in `textPrimary` at the end of the arc. No pointer line, no knurling, no 3D cap.
- Modulation amount: a second arc 6 px outside the main one, 2 px wide, `accent @ 35%`, spanning the modulation range. Visible in Mod view or on hover.
- Density needs special treatment because it has two regions with a dead zone in the middle. Draw a tiny tick at center, and put the region name in the readout ("regular" / "off" / "random").

### 5.3 Buttons
- **Freeze**: a pill, 88 × 32. When off: `tile` fill with a `textSecondary` label. When on: `accentSoft` fill, `accent` label and border, plus the visualizer state change. It is the most important button, so it sits next to the visualizer.
- **Seed**: a pill of the same size that momentarily flashes `accent` (100 ms) when pressed.
- **Quality mode**: a segmented control in the header. Equal-width segments, with the selected segment sliding 150 ms.

### 5.4 Header & footer
- Header: product mark (8 px accent dot plus the name), preset browser with ‹ › arrows in the middle, mode control, and a settings gear on the right. That's all.
- Footer: input meter (thin, 4 px tall bars), input gain as a small inline horizontal slider, and a grain count in `micro`. Low contrast. It should almost disappear.

---

## 6. The grain visualizer (the hero)

This is what people will remember, so most of the polish should go here. Inspiration: the way Imagiro **Autochroma** shows the incoming audio buffer across the top of the UI with grains reacting live on it. Capture that sense of *seeing the grains happen*, in our own visual language.

### 6.1 What it shows
- **The x-axis is the recording buffer**, laid out horizontally. The write head moves left→right (or scrolls), and older audio fades toward the left.
- **The waveform** is a soft, mirrored min/max envelope in `textPrimary @ 14%`, filled, not stroked. It sits behind everything.
- **Each active grain** is a soft rounded capsule or glowing dot:
  - x = the grain's read position in the buffer
  - y = pitch (center line = 0 st, up = higher)
  - width = grain size
  - opacity = the grain's current envelope value (so grains *breathe* in and out)
  - color = the pitch gradient (4.1)
  - The Shape parameter subtly changes the capsule edge softness (sharp attack = crisp edges, smooth = blurred).
- **The write head** is a 1 px vertical line in `textPrimary @ 40%`.
- **The Time parameter** is shown as a faint bracket or tint over the region of the buffer grains are drawn from, so turning Time *visibly* moves where grains appear.

### 6.2 Data flow (real-time safe)
- The audio thread pushes grain events (`pos, pitch, size, env, id`) into a **lock-free FIFO** (`juce::AbstractFifo`). No allocation and no locks on the audio thread.
- The UI drains the FIFO at **60 fps** (`juce::VBlankAttachment`, not a Timer).
- The waveform is a downsampled peak buffer (e.g. 512 columns), also passed through the FIFO.
- Cap the number of drawn grains (e.g. 128) and drop the oldest first.

### 6.3 Rendering
- Draw in one component with `paint()`. Cache the waveform layer as a `juce::Image` and only redraw it when new audio arrives.
- Make grains glow with a cheap fake: draw each grain twice, once large at low alpha and once small at full alpha. No real blur.
- Must stay smooth at 200% scaling. Profile it, and if it costs more than ~2 ms per frame, simplify.

### 6.4 States
- **Idle / no input**: the waveform flatlines and a single line of `micro` text in `textTertiary` says "Waiting for audio". No error styling.
- **Frozen**: the waveform stops scrolling, the panel gets a very subtle `accent @ 4%` tint, a small "FROZEN" `label` appears top-left, and the grains keep moving. Freezing should feel like time stopping, not like the plugin breaking.
- **Hover a tile**: the visualizer softly highlights what that parameter controls (Time → the read region, Size → grain widths, Pitch → the vertical axis guide). This is what makes the plugin teach itself.

---

## 7. Interaction rules

- Drag **vertically** to change a knob. 200 px covers the full range. **Shift** = fine (×0.1).
- **Double-click** (or Alt/Option-click) resets to default.
- **Double-click the value** to type an exact value, with units accepted ("120ms", "+7").
- The mouse wheel adjusts the value. Arrow keys change the focused control, and Tab moves between tiles.
- While dragging, hide the cursor and keep the value readout steady (no jitter).
- Every control works with DAW automation and shows host-automated changes live.
- Tooltips are optional, plain text in `micro`, and appear after 600 ms.

---

## 8. Do / Don't

**Do**
- Use one accent color. Let gray do the rest.
- Keep exact values visible at all times.
- Give things room. When in doubt, add space instead of a border.
- Align everything to the grid. Check alignment in screenshots.
- Make the visualizer react to *every* parameter in some way.

**Don't**
- No gradients on controls, no bevels, glossy highlights, metal, wood or LED skeuomorphism.
- No more than 2 font weights on screen at once (Medium + SemiBold; Regular only for units/micro).
- No ALL CAPS except tile labels.
- No icons where a word is clearer.
- No hardware-panel imitation: no screws, no jacks, no Eurorack rails.
- No pure black (`#000`) or pure white (`#FFF`) as fills.

---

## 9. References — what to take from each

Put screenshots in `design/refs/` and look at them before every UI work session.

| Reference | Take this | Leave this |
|---|---|---|
| **Imagiro Autochroma** | Buffer-across-the-top visualizer; grains visibly reacting in real time; inviting, uncluttered | Its colors and specific shapes |
| **Imagiro Autochroma** — control boxes | Each control lives in its own small, tidy box | Exact box styling |
| **Cableguys HalfTime** | One big central display as the focus, with controls clearly secondary; friendly and confident | Playful/pastel palette |
| **Valhalla VintageVerb** | Extreme restraint: flat, text-forward, few colors, exact numbers everywhere | The dated/retro feel and the sliders-only layout |
| **Wavesfactory** (any) | Polish, generous spacing, soft depth, every detail resolved | Product-specific illustration |
| **Apple** (Logic Pro, macOS Settings) | Typography, gray steps instead of borders, quiet hover states, segmented controls | — |

---

## 10. How to work (agent loop)

1. **Render, don't guess.** After every UI change, build the snapshot target and look at the PNG:
   `./build/UISnapshot --scale 1 --out snapshots/ui@1x.png` (and `--scale 2`).
   The snapshot target creates the editor, feeds it a test signal so the visualizer is populated, and saves `createComponentSnapshot()` to disk. One variant should have Freeze on, and one should show the hover state of a tile.
2. Compare the snapshot with `design/refs/` and with this file. Critique it in writing (alignment, spacing, contrast, hierarchy) **before** changing code.
3. Make one focused change at a time. Snapshot again.
4. Keep `snapshots/` in git so we can compare visual history.

## 11. Definition of done (checklist for every UI PR)

- [ ] No hardcoded colors, sizes or fonts outside `Theme.h`
- [ ] Every element sits on the 4/8 px grid (checked in a 2× snapshot)
- [ ] Looks crisp at 100% and 200%, with no blurry text or lines
- [ ] Values use tabular numbers and don't jitter while dragging
- [ ] Visualizer runs at 60 fps with no allocation on the audio thread
- [ ] Freeze, idle and hover states all implemented and snapshotted
- [ ] Double-click reset, Shift fine-drag and typed value entry all work
- [ ] Only one accent color is visible on screen
- [ ] Orfeo has seen the snapshot

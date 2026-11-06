#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Flat drawing primitives and the panel grid for Pebble (after Mutable
 * Instruments' Beads). Same design language as Skýjað's panel: aluminium
 * and black print, mono tracked type, faders instead of knobs, colour
 * used only to mean "this control belongs to that section" or "this is a
 * live signal" - never decoration.
 *
 * Everything is expressed in a fixed design space; the editor applies one
 * scale transform, so layout constants double as hit-test geometry.
 */
namespace bds::panel
{

constexpr float designW = 970.0f, designH = 350.0f;

/* ── Palette ─────────────────────────────────────────────────────────────── */
namespace hue
{
const juce::Colour paper   { 0xffeef0ec };   // brushed aluminium
const juce::Colour ink     { 0xff15181b };
const juce::Colour texture { 0xff2399a6 };   // teal - the five main controls
const juce::Colour engine  { 0xffc17d1f };   // amber - transport/mode
const juce::Colour mod     { 0xff7a5ea8 };   // violet - randomize-amount pots
const juce::Colour mix     { 0xffab2f6e };   // magenta - output blend
const juce::Colour freeze  { 0xffd6452f };   // red, lit
const juce::Colour signal  { 0xff2e7d52 };
const juce::Colour clip    { 0xffc02617 };
}

inline juce::Colour ink (float alpha) { return hue::ink.withAlpha (alpha); }

/* ── Grid ────────────────────────────────────────────────────────────────
 * Four columns: TEXTURE (the five direct knobs), ENGINE (transport, media
 * quality, freeze/seed), MOD (the four randomize-amount pots), MIX (the
 * three blend knobs). Same proportions as Skýjað's three-column grid with
 * one extra column inserted, same rule-line rhythm.
 */
constexpr float contentL = 40.0f, contentR = 930.0f;
constexpr float rule1X = 316.0f, rule2X = 510.0f, rule3X = 754.0f;
constexpr float ruleTop = 20.0f, ruleBot = 240.0f;

constexpr float titleY = 22.0f, titleSize = 9.5f, titleTracking = 3.0f;

constexpr float faderStep  = 50.0f;
constexpr float faderTop   = 78.0f;
constexpr float faderTrack = 130.0f;
constexpr float faderBot   = faderTop + faderTrack;

constexpr float textureX = 44.0f;                 // 5 faders, ends 294
constexpr float engineL = 338.0f, engineW = 150.0f; // ends 488
constexpr float modX = 532.0f;                      // 4 faders, ends 732
constexpr float mixX = 776.0f;                       // 3 faders, ends 926

constexpr float statusY = 320.0f, statusRuleY = 304.0f;

inline float faderX (float columnX, int i) { return columnX + faderStep * (0.5f + (float) i); }

/** Full-width live grain-cloud visualizer band below the four columns. */
inline juce::Rectangle<float> grainCloudRect() { return { contentL, 246.0f, contentR - contentL, 50.0f }; }

/** Four-position vertical selector (MEDIA quality). */
constexpr float listW = 110.0f, listRow = 17.0f;
inline juce::Rectangle<float> mediaRect() { return { engineL + (engineW - listW) * 0.5f, 92.0f, listW, listRow * 4.0f }; }

/** FREEZE / SEED / SEED LOCK / MONO GRAIN / OUTPUT GATE stack at the top of
    the engine column, one row each, below the section title line. */
inline juce::Rectangle<float> freezeRect()    { return { engineL, 36.0f, engineW, 15.0f }; }
inline juce::Rectangle<float> seedRect()      { return { engineL, 55.0f, engineW, 15.0f }; }
inline juce::Rectangle<float> seedLockRect()  { return { engineL, 174.0f, engineW, 14.0f }; }
inline juce::Rectangle<float> monoGrainRect() { return { engineL, 194.0f, engineW, 14.0f }; }
inline juce::Rectangle<float> outputGateRect(){ return { engineL, 214.0f, engineW, 14.0f }; }

/* ── Type ────────────────────────────────────────────────────────────────── */
juce::Font mono (float h, bool bold = false);

void text (juce::Graphics&, const juce::String&, juce::Rectangle<float>,
           float size, juce::Colour,
           juce::Justification = juce::Justification::centred, bool bold = true);

void tracked (juce::Graphics&, const juce::String&, juce::Rectangle<float>,
              float size, juce::Colour, float tracking,
              bool leftAlign = true, bool bold = true);

void rule (juce::Graphics&, float x, float y, float w, float h, float alpha = 0.13f);

/* ── Controls ────────────────────────────────────────────────────────────── */

void fader (juce::Graphics&, float cx, float norm, juce::Colour,
            const juce::String& label, const juce::String& readout,
            bool fromCentre = false);

void selector (juce::Graphics&, juce::Rectangle<float>, int selected,
               juce::Colour, const juce::StringArray& labels, float size = 9.0f);

void selectorVertical (juce::Graphics&, juce::Rectangle<float>, int selected,
                       juce::Colour, const juce::StringArray& labels, float size = 9.0f);

/** Latching or momentary button, lit while it is on. */
void button (juce::Graphics&, juce::Rectangle<float>, bool on,
             juce::Colour, const juce::String& label);

/** A labelled row with a small checkbox at its right edge - SEED LOCK / MONO
    GRAIN / OUTPUT GATE read better as settings than as buttons. */
void toggleRow (juce::Graphics&, juce::Rectangle<float>, bool on,
                juce::Colour, const juce::String& label);

void meter (juce::Graphics&, juce::Rectangle<float>, float level, juce::Colour);

void checkbox (juce::Graphics&, juce::Rectangle<float>, bool on, juce::Colour);

void ledBar (juce::Graphics&, float x, float cy, float level, juce::Colour);

void lamp (juce::Graphics&, float cx, float cy, bool on, juce::Colour);

} // namespace bds::panel

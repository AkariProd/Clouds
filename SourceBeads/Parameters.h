#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Parameter layout for the Beads port ("Pebble"). Knob set and laws mirror
// the hardware panel as closely as a VST host allows:
//  - TIME / DENSITY / PITCH / SIZE / SHAPE: the five direct knobs.
//  - *_AMOUNT: the four bipolar attenuverter pots under TIME/SIZE/SHAPE/PITCH.
//    On hardware, with nothing patched into the matching CV jack, these
//    pots control the DEPTH OF INTERNAL RANDOM MODULATION of that parameter
//    rather than classic CV scaling (see RandomizedParameter in the firmware).
//    No VST host exposes a literal CV jack, so we keep that exact unpatched
//    behaviour permanently: these knobs are "randomize amount" knobs.
//  - FEEDBACK / DRY-WET / REVERB: the three mix knobs.
//  - FREEZE: latches the buffer (infinite sustain).
//  - SEED / SEED LOCK: the front-panel seed button and its hold-lock.
//  - MEDIA: the 4-way quality switch (Dirty Tape / Clean Tape /
//    Vintage Digital / Digital) — same order as the firmware's Media enum.
//  - MONO GRAIN: the monophonic grain-stealing mode (advanced feature on
//    hardware, exposed here as a plain toggle).
//  - OUTPUT GATE: replaces the right output with a trigger pulse train,
//    exactly like the hardware's gate-output mode.
//
// Simplification vs. hardware (documented, not hidden): no literal per-
// parameter CV inputs are exposed (a DAW has no CV jacks); host automation
// of the main knobs is the modulation path instead. Stereo/mono delay
// memory ("mono_" in the firmware) is left permanently stereo.

namespace bds
{

namespace pid
{
extern const juce::String time;
extern const juce::String timeAmount;
extern const juce::String density;
extern const juce::String pitch;
extern const juce::String pitchAmount;
extern const juce::String size;
extern const juce::String sizeAmount;
extern const juce::String shape;
extern const juce::String shapeAmount;

extern const juce::String feedback;
extern const juce::String dryWet;
extern const juce::String reverb;

extern const juce::String freeze;
extern const juce::String seed;
extern const juce::String seedLock;

extern const juce::String media;
extern const juce::String monoGrain;
extern const juce::String outputGate;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

/** Hardware's PotChannel::LAW_ATTENUVERTER: dead zone at centre, roughly
    quadratic taper either side. Input 0..1 (raw knob), output ~-1..1. */
float attenuverterLaw (float knob01);

/** Hardware's PotChannel::LAW_DETUNE: maps a 0..1 knob position onto
    musically-spaced intervals (0, 1, 3, 4, 7, 12, 24 semitones), crossfaded,
    mirrored around centre. Matches beads::CvReader's PITCH pot exactly. */
float detuneLaw (float knob01);

} // namespace bds

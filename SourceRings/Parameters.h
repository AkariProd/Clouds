#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Parameter layout for the Rings port ("Chime"). Knob set and laws mirror
// the hardware panel: FREQUENCY / STRUCTURE / BRIGHTNESS / DAMPING /
// POSITION as the five direct knobs, each with a bipolar attenuverter
// ("*_CV_AMOUNT") that scales a matching automatable "*_CV" modulation
// input — unlike Beads, Rings' attenuverter is a plain CV scaler with no
// special unpatched-mode behaviour (cv_scaler.cc's ATTENUVERT macro always
// computes pot + attenuverter*cv; with cv == 0, that's just the pot value,
// exactly like nothing patched in on hardware).
//
// MODEL selects one of the six resonator voices firmware exposes; POLYPHONY
// is 1-4 voices; STRUM MODE folds the hardware's internal_strum/
// internal_note/internal_exciter trio into one control:
//   - External Trigger: a MIDI note-on strums (nothing patched into STRUM).
//   - Note Change: strums whenever the MIDI note changes (matches hardware
//     with something patched into V/OCT but not STRUM).
//   - Audio Onset: a real onset detector (ported verbatim) listens to the
//     host audio input and strums on a detected transient (matches hardware
//     with nothing patched into V/OCT or STRUM, playing by ear).
// MIDI notes always drive V/OCT (performance_state.note/tonic) when held;
// with no note held, pitch is free-running from the FREQUENCY knob alone,
// exactly like nothing patched into V/OCT on hardware.

namespace rng
{

namespace pid
{
extern const juce::String frequency;
extern const juce::String frequencyAmount;
extern const juce::String frequencyCv;
extern const juce::String structure;
extern const juce::String structureAmount;
extern const juce::String structureCv;
extern const juce::String brightness;
extern const juce::String brightnessAmount;
extern const juce::String brightnessCv;
extern const juce::String damping;
extern const juce::String dampingAmount;
extern const juce::String dampingCv;
extern const juce::String position;
extern const juce::String positionAmount;
extern const juce::String positionCv;

extern const juce::String model;
extern const juce::String polyphony;
extern const juce::String strumMode;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

/** Hardware's LAW_QUADRATIC_BIPOLAR: centred at 0.5, squared taper either
    side, scaled by 4*3.3. Used for Structure/Brightness/Damping/Position
    attenuverters. */
float quadraticBipolarLaw (float knob01);

/** Hardware's LAW_QUARTIC_BIPOLAR: as above but to the 4th power, steeper
    centre dead feel. Used for the Frequency (FM) attenuverter only. */
float quarticBipolarLaw (float knob01);

} // namespace rng

#pragma once

#include <array>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#if PEBBLE_HAS_INSPECTOR
#include <melatonin_inspector/melatonin_inspector.h>
#endif

#include "Panel.h"
#include "PluginProcessor.h"

/**
 * Pebble's panel: same flat, fader-based design language as Skýjað's
 * (Panel.h), four columns instead of three -- TEXTURE (the five direct
 * knobs), ENGINE (transport / media quality), MOD (the four randomize-
 * amount pots), MIX (the three blend knobs). One scale transform, fixed
 * design-space hit testing, no juce::Slider anywhere.
 */
class PebbleEditor final : public juce::AudioProcessorEditor,
                            private juce::Timer
{
public:
    explicit PebbleEditor (PebbleProcessor&);
    ~PebbleEditor() override;

    void paint (juce::Graphics&) override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;

    enum class Kind { fader, selector, latch, momentary };

    struct Ctl
    {
        Kind kind {};
        juce::RangedAudioParameter* param = nullptr;
        juce::Rectangle<float> hit;
        float travel = 150.0f;
        int   positions = 0;
        bool  vertical = false;
    };

    void buildControls();

    float scale() const;
    juce::Point<float> toDesign (juce::Point<float> px) const;
    int controlAt (juce::Point<float> design) const;

    static int indexOf (const juce::RangedAudioParameter& p, int positions);

    /** Live "grain cloud" visualizer: a small UI-side particle system driven
        by the current parameter values and the live signal level -- not a
        literal readout of DSP buffer contents, a reactive one. Each particle
        is a single rendered "grain": spawn rate from DENSITY, size from
        SIZE, softness from SHAPE, hue from PITCH (so turning the pitch knob
        visibly shifts the cloud's colour -- an "autochrome" scatter rather
        than a plain progress bar), brightness from the live output level.
        Freeze stops new spawns and lets existing ones settle/pulse instead
        of drifting, which reads clearly as "the buffer stopped moving". */
    struct Particle
    {
        float x = 0.0f, y = 0.0f;
        float vx = 0.0f;
        float hue = 0.0f;
        float size = 1.0f;
        float age = 0.0f, life = 1.0f;
        bool  alive = false;
    };
    static constexpr int kMaxParticles = 96;
    std::array<Particle, kMaxParticles> particles_ {};
    juce::Random rng_;
    void updateGrainCloud (float dtSeconds);
    void drawGrainCloud (juce::Graphics&) const;

    PebbleProcessor& proc_;
    std::vector<Ctl> ctls_;

#if PEBBLE_HAS_INSPECTOR
    // Ctrl/Cmd+I toggles the overlay; see DESIGN.md section 10.
    melatonin::Inspector inspector_ { *this };
#endif

    int   dragIdx_ = -1;
    float dragStartNorm_ = 0.0f;
    float dragStartY_ = 0.0f;
    int   momentaryIdx_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PebbleEditor)
};

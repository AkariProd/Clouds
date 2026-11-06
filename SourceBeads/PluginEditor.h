#pragma once

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

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

    PebbleProcessor& proc_;
    std::vector<Ctl> ctls_;

    int   dragIdx_ = -1;
    float dragStartNorm_ = 0.0f;
    float dragStartY_ = 0.0f;
    int   momentaryIdx_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PebbleEditor)
};

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "PebbleLookAndFeel.h"

namespace bds { enum class PillIcon { loop, cloud, cube }; }

/** Rounded colour "pill" switch matching the hardware's small coloured
    mode-toggle strips (icon + yellow LED, teal/orange/magenta). A real
    toggle, not decoration -- bound to a bool parameter via
    juce::ParameterAttachment, same as every other control on this panel. */
class PillSwitch final : public juce::Component
{
public:
    PillSwitch (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                juce::Colour colour, bds::PillIcon icon);

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Colour colour_;
    bds::PillIcon icon_;
    juce::ParameterAttachment attachment_;
    float value_ = 0.0f;
};

/** Panel is a close visual clone of the real Beads hardware photo: same
    control positions (FREEZE/DENSITY/SEED up top, TIME/PITCH below,
    SIZE/SHAPE with their 3 colour pill switches, a row of 7 small trim
    knobs), glossy 3D-shaded knobs/buttons (gradient + highlight + drop
    shadow, not flat fills), chrome-look jacks, and the hardware's dotted
    connector-line + tick-mark decoration. Branded "Pebble" / "SoulMade"
    rather than the original names, per Mutable Instruments' own guidance
    for derivative works -- their CC-BY-SA-licensed panel layout and
    colour accents are recreated, their trademarked wordmark/bust logo is
    not; the header icon and bottom badge are original marks.

    The 3 colour pill switches are real toggles (seedLock / monoGrain /
    outputGate) rather than an exact byte-for-byte reproduction of which
    specific hidden-menu setting lives under which coloured switch on the
    real hardware -- that mapping isn't documented anywhere I could verify,
    so existing functional parameters were assigned to matching switches
    instead of adding new decorative-only controls. */
class PebbleEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PebbleEditor (PebbleProcessor&);
    ~PebbleEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void makeKnob (Knob& k, const juce::String& paramId, const juce::String& text, bool whiteCap);
    static void drawJack (juce::Graphics& g, juce::Point<float> centre, float radius);
    static void drawDotted (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b);

    PebbleProcessor& proc_;
    bds::PebbleLookAndFeel lnf_;

    Knob density_, time_, pitch_, size_, shape_;
    Knob feedback_, dryWet_, reverb_, timeAmount_, sizeAmount_, shapeAmount_, pitchAmount_;

    juce::ToggleButton freezeBtn_ { "FREEZE" };
    juce::ToggleButton seedBtn_ { "SEED" };
    juce::Label freezeLabel_ { {}, "FREEZE" }, seedLabel_ { {}, "SEED" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAtt_, seedAtt_;

    std::unique_ptr<PillSwitch> seedLockPill_, monoGrainPill_, outputGatePill_;

    juce::ComboBox mediaBox_;
    juce::Label mediaLabel_ { {}, "MEDIA" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mediaAtt_;

    // Jack + dotted-line layout anchors, computed in resized().
    juce::Point<float> cvJackCentres_[6];
    juce::Point<float> ioJackCentres_[6];
    juce::Point<float> knobCentres_[6]; // time,size,shape,pitch + the two trim-row end knobs

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PebbleEditor)
};

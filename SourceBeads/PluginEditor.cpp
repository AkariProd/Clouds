#include "PluginEditor.h"

#include <cmath>

using namespace bds;
using namespace bds::panel;

namespace
{
/* Control indices into `ctls_`, in build order. */
constexpr int kTexture     = 0;  // 5: density, time, pitch, size, shape
constexpr int kFreeze      = 5;
constexpr int kSeed        = 6;
constexpr int kMedia       = 7;
constexpr int kSeedLock    = 8;
constexpr int kMonoGrain   = 9;
constexpr int kOutputGate  = 10;
constexpr int kMod         = 11; // 4: time, size, shape, pitch amount
constexpr int kMix         = 15; // 3: feedback, dryWet, reverb

const char* kTextureId[5]    = { "density", "time", "pitch", "size", "shape" };
const char* kTextureLabel[5] = { "DENSITY", "TIME", "PITCH", "SIZE", "SHAPE" };

const char* kModId[4]    = { "timeAmount", "sizeAmount", "shapeAmount", "pitchAmount" };
const char* kModLabel[4] = { "TIME", "SIZE", "SHAPE", "PITCH" };

const char* kMixId[3]    = { "feedback", "dryWet", "reverb" };
const char* kMixLabel[3] = { "FEEDBACK", "DRY/WET", "REVERB" };

const char* kMediaLabel[4] = { "DIRTY TAPE", "CLEAN TAPE", "VINTAGE DIG", "DIGITAL" };

juce::String pct (float norm) { return juce::String (juce::roundToInt (norm * 100.0f)) + "%"; }

juce::String pctSigned (float norm)
{
    const float v = bds::attenuverterLaw (norm) * 100.0f;
    return (v >= 0.0f ? "+" : "") + juce::String (juce::roundToInt (v)) + "%";
}
} // namespace

PebbleEditor::PebbleEditor (PebbleProcessor& p)
    : AudioProcessorEditor (&p), proc_ (p)
{
    setOpaque (true);
    buildControls();

    setResizable (true, true);
    setResizeLimits (640, (int) (640.0f * designH / designW),
                     1600, (int) (1600.0f * designH / designW));
    getConstrainer()->setFixedAspectRatio ((double) designW / (double) designH);
    setSize ((int) designW, (int) designH);

    startTimerHz (30);
}

PebbleEditor::~PebbleEditor() = default;

/* ── Control table ───────────────────────────────────────────────────────── */

void PebbleEditor::buildControls()
{
    auto add = [this] (Kind kind, const juce::String& id, juce::Rectangle<float> hit,
                       float travel, int positions = 0, bool vertical = false)
    {
        Ctl c;
        c.kind = kind;
        c.param = proc_.apvts.getParameter (id);
        c.hit = hit;
        c.travel = travel;
        c.positions = positions;
        c.vertical = vertical;
        jassert (c.param != nullptr);
        ctls_.push_back (c);
    };

    auto faderBox = [] (float cx)
    {
        return juce::Rectangle<float> (cx - 18.0f, faderTop - 8.0f, 36.0f, faderTrack + 16.0f);
    };

    for (int i = 0; i < 5; ++i)
        add (Kind::fader, kTextureId[i], faderBox (faderX (textureX, i)), faderTrack);

    add (Kind::latch,     pid::freeze,     freezeRect(),    0.0f, 2);
    add (Kind::momentary, pid::seed,       seedRect(),      0.0f, 2);
    add (Kind::selector,  pid::media,      mediaRect(),     0.0f, 4, true);
    add (Kind::latch,     pid::seedLock,   seedLockRect(),  0.0f, 2);
    add (Kind::latch,     pid::monoGrain,  monoGrainRect(), 0.0f, 2);
    add (Kind::latch,     pid::outputGate, outputGateRect(),0.0f, 2);

    for (int i = 0; i < 4; ++i)
        add (Kind::fader, kModId[i], faderBox (faderX (modX, i)), faderTrack);

    for (int i = 0; i < 3; ++i)
        add (Kind::fader, kMixId[i], faderBox (faderX (mixX, i)), faderTrack);
}

float PebbleEditor::scale() const { return (float) getWidth() / designW; }

juce::Point<float> PebbleEditor::toDesign (juce::Point<float> px) const
{
    const float k = juce::jmax (0.0001f, scale());
    return { px.x / k, px.y / k };
}

int PebbleEditor::controlAt (juce::Point<float> design) const
{
    for (size_t i = 0; i < ctls_.size(); ++i)
        if (ctls_[i].hit.contains (design))
            return (int) i;
    return -1;
}

int PebbleEditor::indexOf (const juce::RangedAudioParameter& p, int positions)
{
    return juce::jlimit (0, positions - 1,
                         juce::roundToInt (p.getValue() * (float) (positions - 1)));
}

/* ── Interaction ─────────────────────────────────────────────────────────── */

void PebbleEditor::mouseDown (const juce::MouseEvent& e)
{
    const auto d = toDesign (e.position);
    const int idx = controlAt (d);
    if (idx < 0)
        return;

    auto& c = ctls_[(size_t) idx];

    if (c.kind == Kind::momentary)
    {
        c.param->beginChangeGesture();
        c.param->setValueNotifyingHost (1.0f);
        momentaryIdx_ = idx;
        repaint();
        return;
    }

    if (c.kind == Kind::selector || c.kind == Kind::latch)
    {
        const int n = juce::jmax (2, c.positions);
        const float along = c.vertical ? (d.y - c.hit.getY()) / (c.hit.getHeight() / (float) n)
                                       : (d.x - c.hit.getX()) / (c.hit.getWidth()  / (float) n);
        const int seg = juce::jlimit (0, n - 1, (int) along);
        c.param->beginChangeGesture();
        c.param->setValueNotifyingHost (c.kind == Kind::latch
                                            ? (c.param->getValue() > 0.5f ? 0.0f : 1.0f)
                                            : (float) seg / (float) (n - 1));
        c.param->endChangeGesture();
        repaint();
        return;
    }

    dragIdx_ = idx;
    dragStartNorm_ = c.param->getValue();
    dragStartY_ = d.y;
    c.param->beginChangeGesture();
    setMouseCursor (juce::MouseCursor::NoCursor);
}

void PebbleEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragIdx_ < 0)
        return;

    auto& c = ctls_[(size_t) dragIdx_];
    const float sensitivity = e.mods.isShiftDown() ? 0.22f : 1.0f;
    const float delta = (dragStartY_ - toDesign (e.position).y) / c.travel;

    c.param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f,
                                                  dragStartNorm_ + delta * sensitivity));
    repaint();
}

void PebbleEditor::mouseUp (const juce::MouseEvent&)
{
    if (momentaryIdx_ >= 0)
    {
        auto& c = ctls_[(size_t) momentaryIdx_];
        c.param->setValueNotifyingHost (0.0f);
        c.param->endChangeGesture();
        momentaryIdx_ = -1;
        repaint();
    }

    if (dragIdx_ < 0)
        return;

    ctls_[(size_t) dragIdx_].param->endChangeGesture();
    dragIdx_ = -1;
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void PebbleEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int idx = controlAt (toDesign (e.position));
    if (idx < 0)
        return;

    auto& c = ctls_[(size_t) idx];
    if (c.kind != Kind::fader)
        return;

    c.param->beginChangeGesture();
    c.param->setValueNotifyingHost (c.param->getDefaultValue());
    c.param->endChangeGesture();
    repaint();
}

void PebbleEditor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int idx = controlAt (toDesign (e.position));
    if (idx < 0)
        return;

    auto& c = ctls_[(size_t) idx];
    if (c.kind == Kind::latch || c.kind == Kind::momentary)
        return;

    const float gain = c.kind == Kind::selector
                           ? 1.0f / (0.1f * (float) juce::jmax (1, c.positions - 1))
                           : (e.mods.isShiftDown() ? 0.04f : 0.16f);
    const float delta = w.deltaY * (w.isReversed ? -1.0f : 1.0f) * gain;

    c.param->beginChangeGesture();
    c.param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, c.param->getValue() + delta));
    c.param->endChangeGesture();
    repaint();
}

void PebbleEditor::timerCallback() { repaint(); }

/* ── Paint ───────────────────────────────────────────────────────────────── */

void PebbleEditor::paint (juce::Graphics& g)
{
    g.fillAll (hue::paper);
    g.addTransform (juce::AffineTransform::scale (scale()));

    g.setColour (ink (0.22f));
    g.drawRect (juce::Rectangle<float> (16.0f, 16.0f, designW - 32.0f, designH - 32.0f), 1.0f);

    rule (g, rule1X, ruleTop, 1.0f, ruleBot - ruleTop, 0.11f);
    rule (g, rule2X, ruleTop, 1.0f, ruleBot - ruleTop, 0.11f);
    rule (g, rule3X, ruleTop, 1.0f, ruleBot - ruleTop, 0.11f);

    auto sectionTitle = [&g] (const char* t, float x, juce::Colour c)
    {
        tracked (g, t, { x, titleY, 200.0f, 12.0f }, titleSize, c, titleTracking);
    };
    sectionTitle ("TEXTURE", textureX, hue::texture);
    sectionTitle ("ENGINE",  engineL,  hue::engine);
    sectionTitle ("MOD",     modX,     hue::mod);
    sectionTitle ("MIX",     mixX,     hue::mix);

    /* ── Texture ─────────────────────────────────────────────────────────── */
    for (int i = 0; i < 5; ++i)
    {
        auto* param = ctls_[(size_t) (kTexture + i)].param;
        fader (g, faderX (textureX, i), param->getValue(), hue::texture,
               kTextureLabel[i], pct (param->getValue()));
    }

    /* ── Engine ──────────────────────────────────────────────────────────── */
    {
        const bool frozen = ctls_[(size_t) kFreeze].param->getValue() > 0.5f;
        const bool seedHeld = ctls_[(size_t) kSeed].param->getValue() > 0.5f;
        const int media = indexOf (*ctls_[(size_t) kMedia].param, 4);

        button (g, freezeRect(), frozen, hue::freeze, "FREEZE");
        button (g, seedRect(), seedHeld, hue::engine, "SEED");

        tracked (g, "MEDIA", mediaRect().withY (mediaRect().getY() - 16.0f).withHeight (11.0f),
                 8.0f, ink (0.55f), 0.6f, false);
        selectorVertical (g, mediaRect(), media, hue::engine,
                          { kMediaLabel[0], kMediaLabel[1], kMediaLabel[2], kMediaLabel[3] }, 7.6f);

        toggleRow (g, seedLockRect(),   ctls_[(size_t) kSeedLock].param->getValue()   > 0.5f, hue::engine, "SEED LOCK");
        toggleRow (g, monoGrainRect(),  ctls_[(size_t) kMonoGrain].param->getValue()  > 0.5f, hue::engine, "MONO GRAIN");
        toggleRow (g, outputGateRect(), ctls_[(size_t) kOutputGate].param->getValue() > 0.5f, hue::engine, "OUTPUT GATE");
    }

    /* ── Mod ─────────────────────────────────────────────────────────────── */
    for (int i = 0; i < 4; ++i)
    {
        auto* param = ctls_[(size_t) (kMod + i)].param;
        fader (g, faderX (modX, i), param->getValue(), hue::mod,
               kModLabel[i], pctSigned (param->getValue()), true);
    }

    /* ── Mix ─────────────────────────────────────────────────────────────── */
    for (int i = 0; i < 3; ++i)
    {
        auto* param = ctls_[(size_t) (kMix + i)].param;
        fader (g, faderX (mixX, i), param->getValue(), hue::mix,
               kMixLabel[i], pct (param->getValue()));
    }

    /* ── Status bar ──────────────────────────────────────────────────────── */
    {
        rule (g, contentL, statusRuleY, contentR - contentL, 1.0f, 0.18f);

        const float y = statusY;
        const auto& state = proc_.panel;

        text (g, "VU", { 40.0f, y - 7.0f, 20.0f, 14.0f }, 8.0f, ink (0.45f),
              juce::Justification::left, false);
        ledBar (g, 62.0f, y, state.envIn.load (std::memory_order_relaxed) * 3.0f, hue::signal);

        text (g, "OUT", { 124.0f, y - 7.0f, 26.0f, 14.0f }, 8.0f, ink (0.45f),
              juce::Justification::left, false);
        meter (g, { 152.0f, y - 3.5f, 60.0f, 7.0f },
               state.envOut.load (std::memory_order_relaxed) * 3.0f, hue::signal);

        text (g, "CLIP", { 230.0f, y - 7.0f, 30.0f, 14.0f }, 8.0f, ink (0.45f),
              juce::Justification::left, false);
        lamp (g, 270.0f, y, state.inPeak.load (std::memory_order_relaxed) > 0.99f, hue::clip);

        const double hostRate = proc_.getSampleRate();
        text (g, "RATE", { 300.0f, y - 7.0f, 34.0f, 14.0f }, 8.0f, ink (0.45f),
              juce::Justification::left, false);
        text (g, hostRate > 0.0 ? juce::String (hostRate / 1000.0, 1) + " kHz" : juce::String ("-"),
              { 330.0f, y - 7.0f, 90.0f, 14.0f }, 8.5f, hue::ink, juce::Justification::left);

        text (g, "after beads", { 766.0f, y - 7.0f, 84.0f, 14.0f }, 7.4f, ink (0.32f),
              juce::Justification::right, false);
        tracked (g, "PEBBLE", { contentR - 70.0f, y - 6.0f, 70.0f, 12.0f }, 8.5f,
                 ink (0.62f), 3.2f);
    }
}

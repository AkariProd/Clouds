#include "Panel.h"

#include <cmath>

namespace bds::panel
{

juce::Font mono (float h, bool bold)
{
    return juce::Font (juce::FontOptions()
                           .withName (juce::Font::getDefaultMonospacedFontName())
                           .withHeight (h)
                           .withStyle (bold ? "Bold" : "Regular"));
}

void text (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r,
           float size, juce::Colour c, juce::Justification j, bool bold)
{
    g.setColour (c);
    g.setFont (mono (size, bold));
    g.drawFittedText (s, r.getSmallestIntegerContainer(), j, 1, 0.9f);
}

void tracked (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r,
              float size, juce::Colour c, float tracking, bool leftAlign, bool bold)
{
    g.setColour (c);
    g.setFont (mono (size, bold));
    const auto f = g.getCurrentFont();

    float total = -tracking;
    for (int i = 0; i < s.length(); ++i)
        total += juce::GlyphArrangement::getStringWidth (f, s.substring (i, i + 1)) + tracking;

    float x = leftAlign ? r.getX() : r.getCentreX() - total * 0.5f;
    for (int i = 0; i < s.length(); ++i)
    {
        const auto ch = s.substring (i, i + 1);
        const float w = juce::GlyphArrangement::getStringWidth (f, ch);
        g.drawText (ch, juce::Rectangle<float> (x, r.getY(), w, r.getHeight()),
                    juce::Justification::centred, false);
        x += w + tracking;
    }
}

void rule (juce::Graphics& g, float x, float y, float w, float h, float alpha)
{
    g.setColour (ink (alpha));
    g.fillRect (x, y, w, h);
}

void fader (juce::Graphics& g, float cx, float norm, juce::Colour colour,
            const juce::String& label, const juce::String& readout, bool fromCentre)
{
    constexpr float trackW = 6.0f, handleW = 14.0f, handleH = 4.0f;
    const float y      = faderBot - juce::jlimit (0.0f, 1.0f, norm) * faderTrack;
    const float origin = faderBot - (fromCentre ? 0.5f : 0.0f) * faderTrack;

    tracked (g, label, { cx - 32.0f, faderTop - 24.0f, 64.0f, 11.0f }, 8.0f,
             ink (0.55f), 0.6f, false);

    g.setColour (ink (0.13f));
    for (int i = 0; i <= 8; ++i)
    {
        const float ty = faderTop + faderTrack * (float) i / 8.0f;
        g.fillRect (cx - 14.0f, ty, (i % 4 == 0) ? 5.0f : 3.0f, 1.0f);
    }

    g.setColour (ink (0.15f));
    g.fillRoundedRectangle (cx - trackW * 0.5f, faderTop, trackW, faderTrack, trackW * 0.5f);

    if (std::abs (origin - y) > 1.0f)
    {
        g.setColour (colour);
        g.fillRoundedRectangle (cx - trackW * 0.5f, juce::jmin (y, origin), trackW,
                                std::abs (origin - y), trackW * 0.5f);
    }
    if (fromCentre)
    {
        g.setColour (ink (0.30f));
        g.fillRect (cx + 9.0f, origin - 0.5f, 5.0f, 1.0f);
    }

    g.setColour (hue::ink);
    g.fillRect (cx - handleW * 0.5f, y - handleH * 0.5f, handleW, handleH);

    text (g, readout, { cx - 32.0f, faderBot + 11.0f, 64.0f, 13.0f }, 9.5f, colour);
}

void box (juce::Graphics& g, juce::Rectangle<float> r, float norm, juce::Colour colour,
          const juce::String& label, const juce::String& valueText, const juce::String& unitText,
          bool fromCentre)
{
    constexpr float corner = 7.0f;

    g.setColour (hue::paper.darker (0.025f));
    g.fillRoundedRectangle (r, corner);

    // Soft fill tracking the value (from the bottom, or from the centre for
    // bipolar MOD boxes) -- position reads at a glance, the number confirms.
    const float n = juce::jlimit (0.0f, 1.0f, norm);
    const float origin = fromCentre ? 0.5f : 0.0f;
    const float fillTop = r.getBottom() - r.getHeight() * juce::jmax (n, origin);
    const float fillBot = r.getBottom() - r.getHeight() * juce::jmin (n, origin);
    if (fillBot - fillTop > 0.5f)
    {
        auto clip = r.withTop (fillTop).withBottom (fillBot);
        juce::Path fillPath;
        const bool touchesBottom = fillBot >= r.getBottom() - 0.5f;
        fillPath.addRoundedRectangle (clip.getX(), clip.getY(), clip.getWidth(), clip.getHeight(),
                                       corner, corner, false, false, touchesBottom, touchesBottom);
        g.setColour (colour.withAlpha (0.16f));
        g.fillPath (fillPath);
    }

    g.setColour (ink (0.16f));
    g.drawRoundedRectangle (r, corner, 1.0f);

    // Corner-bracket accent, top-left and top-right.
    g.setColour (colour.withAlpha (0.85f));
    constexpr float bl = 7.0f;
    for (float side : { 0.0f, 1.0f })
    {
        const float x = side == 0.0f ? r.getX() + 3.0f : r.getRight() - 3.0f;
        const float dir = side == 0.0f ? 1.0f : -1.0f;
        g.drawLine (x, r.getY() + 3.0f, x, r.getY() + 3.0f + bl, 1.3f);
        g.drawLine (x, r.getY() + 3.0f, x + dir * bl, r.getY() + 3.0f, 1.3f);
    }

    auto valueArea = r.reduced (4.0f, 3.0f);
    if (unitText.isNotEmpty())
    {
        auto unitArea = valueArea.removeFromBottom (valueArea.getHeight() * 0.3f);
        text (g, unitText, unitArea, juce::jmin (8.5f, r.getHeight() * 0.16f), ink (0.4f),
              juce::Justification::centred, false);
    }
    text (g, valueText, valueArea, juce::jmin (15.0f, r.getHeight() * 0.3f), ink (0.82f),
          juce::Justification::centred, true);

    text (g, label, { r.getX() - 8.0f, r.getBottom() + 3.0f, r.getWidth() + 16.0f, 12.0f },
          7.6f, ink (0.5f), juce::Justification::centred, false);
}

void selector (juce::Graphics& g, juce::Rectangle<float> r, int selected,
               juce::Colour colour, const juce::StringArray& labels, float size)
{
    const int n = juce::jmax (1, labels.size());
    const float segW = r.getWidth() / (float) n;

    for (int i = 0; i < n; ++i)
    {
        auto seg = juce::Rectangle<float> (r.getX() + segW * (float) i, r.getY(), segW, r.getHeight());
        if (i == selected)
        {
            g.setColour (colour);
            g.fillRect (seg);
            text (g, labels[i], seg, size, hue::paper);
        }
        else
        {
            text (g, labels[i], seg, size, ink (0.5f));
        }
    }

    g.setColour (ink (0.28f));
    g.drawRect (r, 1.0f);
    g.setColour (ink (0.18f));
    for (int i = 1; i < n; ++i)
        g.fillRect (r.getX() + segW * (float) i, r.getY(), 1.0f, r.getHeight());
}

void selectorVertical (juce::Graphics& g, juce::Rectangle<float> r, int selected,
                       juce::Colour colour, const juce::StringArray& labels, float size)
{
    const int n = juce::jmax (1, labels.size());
    const float segH = r.getHeight() / (float) n;

    for (int i = 0; i < n; ++i)
    {
        auto seg = juce::Rectangle<float> (r.getX(), r.getY() + segH * (float) i, r.getWidth(), segH);
        if (i == selected)
        {
            g.setColour (colour);
            g.fillRect (seg);
            text (g, labels[i], seg, size, hue::paper);
        }
        else
        {
            text (g, labels[i], seg, size, ink (0.5f));
        }
    }

    g.setColour (ink (0.28f));
    g.drawRect (r, 1.0f);
    g.setColour (ink (0.18f));
    for (int i = 1; i < n; ++i)
        g.fillRect (r.getX(), r.getY() + segH * (float) i, r.getWidth(), 1.0f);
}

void button (juce::Graphics& g, juce::Rectangle<float> r, bool on,
             juce::Colour colour, const juce::String& label)
{
    constexpr float corner = 4.0f;
    if (on)
    {
        g.setColour (colour);
        g.fillRoundedRectangle (r, corner);
        tracked (g, label, r.withY (r.getY() + (r.getHeight() - 8.0f) * 0.5f).withHeight (8.0f),
                 7.0f, hue::paper, 0.9f, false);
    }
    else
    {
        g.setColour (hue::paper.darker (0.02f));
        g.fillRoundedRectangle (r, corner);
        tracked (g, label, r.withY (r.getY() + (r.getHeight() - 8.0f) * 0.5f).withHeight (8.0f),
                 7.0f, ink (0.62f), 0.9f, false);
    }

    g.setColour (on ? colour : ink (0.22f));
    g.drawRoundedRectangle (r, corner, 1.0f);
}

void toggleRow (juce::Graphics& g, juce::Rectangle<float> r, bool on,
                juce::Colour colour, const juce::String& label)
{
    auto box = r.removeFromRight (r.getHeight()).reduced (1.5f);
    text (g, label, r.reduced (0.0f, 1.0f), 7.4f, on ? colour : ink (0.5f),
          juce::Justification::left, false);
    checkbox (g, box, on, colour);
}

void meter (juce::Graphics& g, juce::Rectangle<float> r, float level, juce::Colour colour)
{
    g.setColour (ink (0.11f));
    g.fillRect (r);
    g.setColour (colour);
    g.fillRect (r.withWidth (r.getWidth() * juce::jlimit (0.0f, 1.0f, level)));
    g.setColour (ink (0.22f));
    g.drawRect (r, 1.0f);
}

void checkbox (juce::Graphics& g, juce::Rectangle<float> r, bool on, juce::Colour colour)
{
    g.setColour (on ? colour : ink (0.30f));
    g.drawRect (r, 1.2f);
    if (on)
    {
        g.setColour (colour);
        g.fillRect (r.reduced (2.5f));
    }
}

void ledBar (juce::Graphics& g, float x, float cy, float level, juce::Colour colour)
{
    constexpr float s = 8.0f, gap = 5.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto led = juce::Rectangle<float> (s, s).withCentre ({ x + (s + gap) * (float) i + s * 0.5f, cy });
        const float lit = juce::jlimit (0.0f, 1.0f, level * 4.0f - (float) i);
        g.setColour (lit > 0.01f ? colour.withAlpha (0.25f + 0.75f * lit) : ink (0.10f));
        g.fillRect (led);
        g.setColour (ink (0.26f));
        g.drawRect (led, 1.0f);
    }
}

void lamp (juce::Graphics& g, float cx, float cy, bool on, juce::Colour colour)
{
    auto s = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ cx, cy });
    g.setColour (on ? colour : ink (0.10f));
    g.fillRect (s);
    g.setColour (ink (0.28f));
    g.drawRect (s, 1.0f);
}

} // namespace bds::panel

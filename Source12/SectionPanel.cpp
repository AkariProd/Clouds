#include "SectionPanel.h"
#include "LookAndFeelP12.h"

SectionPanel::SectionPanel(const juce::String& title, juce::AudioProcessorValueTreeState& apvts,
                            const std::vector<const p12::ParamDef*>& defs)
    : titleText(title)
{
    // Pick a column count that keeps each section roughly square-ish rather
    // than one giant single row -- matches the hardware panel's "grouped
    // knob cluster" look better than a flat list.
    const int n = (int) defs.size();
    columns = juce::jlimit(2, 6, (int) std::ceil(std::sqrt((double) n)));

    for (auto* def : defs)
    {
        auto* c = new ParamControl(apvts, *def);
        controls.add(c);
        addAndMakeVisible(c);
    }

    const int rows = (n + columns - 1) / juce::jmax(1, columns);
    preferredWidth = columns * ParamControl::kWidth + kPadding * 2;
    preferredHeight = kHeaderHeight + rows * ParamControl::kHeight + kPadding * 2;
}

void SectionPanel::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(LookAndFeelP12::panelBg);
    g.fillRoundedRectangle(b, 4.0f);

    auto header = b.removeFromTop((float) kHeaderHeight);
    g.setColour(LookAndFeelP12::sectionHeaderBg);
    g.fillRoundedRectangle(header, 4.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.drawText(titleText.toUpperCase(), header.reduced(8, 0), juce::Justification::centredLeft);

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 1.0f);
}

void SectionPanel::resized()
{
    auto b = getLocalBounds();
    b.removeFromTop(kHeaderHeight);
    b.reduce(kPadding, kPadding);

    int x = b.getX();
    int y = b.getY();
    int col = 0;
    for (auto* c : controls)
    {
        c->setBounds(x, y, ParamControl::kWidth, ParamControl::kHeight);
        x += ParamControl::kWidth;
        if (++col >= columns)
        {
            col = 0;
            x = b.getX();
            y += ParamControl::kHeight;
        }
    }
}

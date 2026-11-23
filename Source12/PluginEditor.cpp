#include "PluginEditor.h"
#include <cstring>

Seer12PanelContent::Seer12PanelContent(juce::AudioProcessorValueTreeState& apvts)
{
    // One shared Oscillator section (with OSC 1-4 select buttons) replaces
    // the 4 separate per-oscillator sections -- placed first.
    auto* oscSection = new OscillatorSection("Oscillators", apvts);
    ownedSections.add(oscSection);
    layoutPanels.push_back(oscSection);
    addAndMakeVisible(oscSection);

    for (const auto& sec : p12::kSections)
    {
        if (std::strcmp(sec.id, "OSC1") == 0 || std::strcmp(sec.id, "OSC2") == 0
            || std::strcmp(sec.id, "OSC3") == 0 || std::strcmp(sec.id, "OSC4") == 0)
            continue;  // handled by the single OscillatorSection above

        std::vector<const p12::ParamDef*> defs;
        for (const auto& def : p12::kParams)
            if (std::strcmp(def.section, sec.id) == 0)
                defs.push_back(&def);
        if (defs.empty())
            continue;
        auto* panel = new SectionPanel(sec.title, apvts, defs);
        ownedSections.add(panel);
        layoutPanels.push_back(panel);
        addAndMakeVisible(panel);
    }
    setSize(kContentWidth, 100);  // real height computed by resized() below
}

void Seer12PanelContent::paint(juce::Graphics& g)
{
    g.fillAll(LookAndFeelP12::panelBgDark);
}

void Seer12PanelContent::resized()
{
    // Simple left-to-right, top-to-bottom flow layout of section boxes,
    // wrapping to a new row when the next panel would overflow the width.
    int x = kMargin, y = kMargin, rowHeight = 0;
    for (auto* panel : layoutPanels)
    {
        const int w = panel->getPreferredWidth();
        const int h = panel->getPreferredHeight();

        if (x + w + kMargin > kContentWidth && x > kMargin)
        {
            x = kMargin;
            y += rowHeight + kMargin;
            rowHeight = 0;
        }

        if (auto* comp = dynamic_cast<juce::Component*>(panel))
            comp->setBounds(x, y, w, h);
        x += w + kMargin;
        rowHeight = juce::jmax(rowHeight, h);
    }

    setSize(kContentWidth, y + rowHeight + kMargin);
}

Seer12Editor::Seer12Editor(Seer12Processor& p)
    : AudioProcessorEditor(&p), processor(p), content(p.apvts), midiMonitor(p)
{
    setLookAndFeel(&lookAndFeel);

    titleLabel.setText("SEER 12 -- Hardware Synth Editor", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, LookAndFeelP12::accent);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    addAndMakeVisible(midiMonitor);

    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    setResizable(true, true);
    setSize(1200, 900);
}

Seer12Editor::~Seer12Editor()
{
    setLookAndFeel(nullptr);
}

void Seer12Editor::paint(juce::Graphics& g)
{
    g.fillAll(LookAndFeelP12::panelBgDark);
}

void Seer12Editor::resized()
{
    auto b = getLocalBounds();
    titleLabel.setBounds(b.removeFromTop(32).reduced(10, 4));
    midiMonitor.setBounds(b.removeFromTop(92).reduced(8, 2));
    viewport.setBounds(b);
}

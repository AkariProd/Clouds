#include "PluginEditor.h"
#include <cstring>

Seer12PanelContent::Seer12PanelContent(juce::AudioProcessorValueTreeState& apvts)
{
    // Groups of numbered instances (oscillators, LFOs, delays, envelopes,
    // mod-matrix slots) each collapse into ONE section with select buttons
    // instead of N separate full-knob sections -- mirrors how the Prophet
    // 12's own panel reuses shared control rows across numbered instances.
    auto addMultiInstance = [this, &apvts](const juce::String& title, std::vector<int> nums,
                                            std::vector<juce::String> prefixes, const juce::String& buttonPrefix)
    {
        auto* sec = new MultiInstanceSection(title, apvts, nums, prefixes, buttonPrefix);
        ownedSections.add(sec);
        layoutPanels.push_back(sec);
        addAndMakeVisible(sec);
    };

    addMultiInstance("Oscillators", { 1, 2, 3, 4 }, { "Osc" }, "OSC");
    addMultiInstance("LFOs", { 1, 2, 3, 4 }, { "LFO" }, "LFO");
    addMultiInstance("Delays", { 1, 2, 3, 4 }, { "Delay" }, "DELAY");
    addMultiInstance("Envelope 3 / 4", { 3, 4 }, { "Envelope", "Env" }, "ENV");
    addMultiInstance("Modulation Matrix", { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }, { "Mod" }, "MOD");

    const std::vector<const char*> handledElsewhere = {
        "OSC1", "OSC2", "OSC3", "OSC4", "LFO1", "LFO2", "LFO3", "LFO4",
        "DELAY1", "DELAY2", "DELAY3", "DELAY4", "ENV3", "ENV4", "MODMATRIX"
    };

    for (const auto& sec : p12::kSections)
    {
        bool handled = false;
        for (const auto* id : handledElsewhere)
            if (std::strcmp(sec.id, id) == 0) { handled = true; break; }
        if (handled)
            continue;

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
    : AudioProcessorEditor(&p), processor(p), content(p.apvts), midiMonitor(p), sysexCapture(p)
{
    setLookAndFeel(&lookAndFeel);

    titleLabel.setText("SEER 12 -- Hardware Synth Editor", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, LookAndFeelP12::accent);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    addAndMakeVisible(midiMonitor);
    addAndMakeVisible(sysexCapture);

    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    setResizable(true, true);
    setSize(1200, 950);
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
    sysexCapture.setBounds(b.removeFromTop(64).reduced(8, 2));
    viewport.setBounds(b);
}

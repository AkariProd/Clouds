#include "OscillatorSection.h"
#include "LookAndFeelP12.h"
#include <cmath>

OscillatorSection::OscillatorSection(const juce::String& title, juce::AudioProcessorValueTreeState& apvtsRef)
    : titleText(title)
{
    struct FieldEntry
    {
        juce::String label;
        std::array<const p12::ParamDef*, 4> defs { nullptr, nullptr, nullptr, nullptr };
    };
    std::vector<FieldEntry> fields;

    // Group "Osc 1 Pitch" / "Osc 2 Pitch" / ... into one "Pitch" field
    // shared across all 4 oscillators, in the order they first appear in
    // the generated table (i.e. Osc 1's field order).
    for (const auto& def : p12::kParams)
    {
        juce::String name(def.name);
        if (!name.startsWith("Osc ") || name.length() < 6 || !juce::CharacterFunctions::isDigit(name[4]))
            continue;
        const int oscIndex = name[4] - '0';
        if (oscIndex < 1 || oscIndex > 4)
            continue;
        const juce::String field = name.substring(6);

        FieldEntry* entry = nullptr;
        for (auto& f : fields)
            if (f.label == field) { entry = &f; break; }
        if (entry == nullptr)
        {
            fields.push_back({ field, {} });
            entry = &fields.back();
        }
        entry->defs[(size_t) (oscIndex - 1)] = &def;
    }

    for (int i = 0; i < 4; ++i)
    {
        auto btn = std::make_unique<juce::TextButton>("OSC " + juce::String(i + 1));
        btn->setClickingTogglesState(true);
        btn->setToggleState(selection[(size_t) i], juce::dontSendNotification);
        btn->setColour(juce::TextButton::buttonOnColourId, LookAndFeelP12::accent);
        btn->onClick = [this, i] { toggleOsc(i); };
        addAndMakeVisible(*btn);
        oscButtons[(size_t) i] = std::move(btn);
    }

    for (auto& f : fields)
    {
        auto* c = new MultiOscParamControl(apvtsRef, f.label, f.defs, selection);
        controls.add(c);
        addAndMakeVisible(c);
    }

    const int n = (int) controls.size();
    columns = juce::jlimit(3, 8, (int) std::ceil(std::sqrt((double) n)) + 1);
    const int rows = (n + columns - 1) / juce::jmax(1, columns);
    preferredWidth = juce::jmax(columns * MultiOscParamControl::kWidth, 4 * 76) + kPadding * 2;
    preferredHeight = kHeaderHeight + kSelectRowHeight + rows * MultiOscParamControl::kHeight + kPadding * 2;
}

void OscillatorSection::toggleOsc(int index)
{
    selection[(size_t) index] = !selection[(size_t) index];

    bool anySelected = false;
    for (bool s : selection)
        anySelected |= s;
    if (!anySelected)
        selection[(size_t) index] = true;  // always keep at least one oscillator selected

    oscButtons[(size_t) index]->setToggleState(selection[(size_t) index], juce::dontSendNotification);
    for (auto* c : controls)
        c->refreshFromSelection();
}

void OscillatorSection::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(LookAndFeelP12::panelBg);
    g.fillRoundedRectangle(b, 4.0f);

    auto header = b.removeFromTop((float) kHeaderHeight);
    g.setColour(LookAndFeelP12::sectionHeaderBg);
    g.fillRoundedRectangle(header, 4.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.drawText(titleText.toUpperCase() + "   (select which to edit ->)", header.reduced(8, 0),
                juce::Justification::centredLeft);

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 1.0f);
}

void OscillatorSection::resized()
{
    auto b = getLocalBounds();
    b.removeFromTop(kHeaderHeight);

    auto selectRow = b.removeFromTop(kSelectRowHeight).reduced(kPadding, 2);
    const int bw = selectRow.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        oscButtons[(size_t) i]->setBounds(selectRow.removeFromLeft(bw).reduced(2));

    b.reduce(kPadding, kPadding);
    int x = b.getX();
    int y = b.getY();
    int col = 0;
    for (auto* c : controls)
    {
        c->setBounds(x, y, MultiOscParamControl::kWidth, MultiOscParamControl::kHeight);
        x += MultiOscParamControl::kWidth;
        if (++col >= columns)
        {
            col = 0;
            x = b.getX();
            y += MultiOscParamControl::kHeight;
        }
    }
}

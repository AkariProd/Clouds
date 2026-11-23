#include "MultiInstanceSection.h"
#include "LookAndFeelP12.h"
#include <cmath>

MultiInstanceSection::MultiInstanceSection(const juce::String& title, juce::AudioProcessorValueTreeState& apvtsRef,
                                            const std::vector<int>& instanceNumbers,
                                            const std::vector<juce::String>& namePrefixes,
                                            const juce::String& buttonPrefix)
    : titleText(title), numInstances((int) instanceNumbers.size())
{
    selection.assign((size_t) numInstances, false);
    if (numInstances > 0)
        selection[0] = true;  // first instance selected by default

    struct FieldEntry
    {
        juce::String label;
        std::vector<const p12::ParamDef*> defs;
    };
    std::vector<FieldEntry> fields;

    for (const auto& def : p12::kParams)
    {
        const juce::String name(def.name);

        int matchedInstanceIdx = -1;
        juce::String fieldLabel;

        for (size_t ii = 0; ii < instanceNumbers.size() && matchedInstanceIdx < 0; ++ii)
        {
            for (const auto& prefix : namePrefixes)
            {
                const juce::String needle = prefix + " " + juce::String(instanceNumbers[ii]) + " ";
                const int pos = name.indexOf(needle);
                if (pos < 0)
                    continue;
                matchedInstanceIdx = (int) ii;
                fieldLabel = (name.substring(0, pos) + name.substring(pos + needle.length())).trim();
                fieldLabel = fieldLabel.replace("  ", " ");
                break;
            }
        }

        if (matchedInstanceIdx < 0)
            continue;

        FieldEntry* entry = nullptr;
        for (auto& f : fields)
            if (f.label == fieldLabel) { entry = &f; break; }
        if (entry == nullptr)
        {
            fields.push_back({ fieldLabel, std::vector<const p12::ParamDef*>((size_t) numInstances, nullptr) });
            entry = &fields.back();
        }
        entry->defs[(size_t) matchedInstanceIdx] = &def;
    }

    buttonColumns = juce::jmin(numInstances, 8);
    buttonRows = (numInstances + buttonColumns - 1) / juce::jmax(1, buttonColumns);

    for (int i = 0; i < numInstances; ++i)
    {
        auto btn = std::make_unique<juce::TextButton>(buttonPrefix + " " + juce::String(instanceNumbers[(size_t) i]));
        btn->setClickingTogglesState(true);
        btn->setToggleState(selection[(size_t) i], juce::dontSendNotification);
        btn->setColour(juce::TextButton::buttonOnColourId, LookAndFeelP12::accent);
        btn->onClick = [this, i] { toggleInstance(i); };
        addAndMakeVisible(*btn);
        instanceButtons.push_back(std::move(btn));
    }

    for (auto& f : fields)
    {
        auto* c = new MultiInstanceParamControl(apvtsRef, f.label, f.defs, selection);
        controls.add(c);
        addAndMakeVisible(c);
    }

    const int n = (int) controls.size();
    columns = juce::jlimit(3, 8, (int) std::ceil(std::sqrt((double) n)) + 1);
    const int rows = (n + columns - 1) / juce::jmax(1, columns);
    preferredWidth = juce::jmax(columns * MultiInstanceParamControl::kWidth, buttonColumns * 76) + kPadding * 2;
    preferredHeight = kHeaderHeight + buttonRows * kSelectRowHeight
                      + rows * MultiInstanceParamControl::kHeight + kPadding * 2;
}

void MultiInstanceSection::toggleInstance(int index)
{
    selection[(size_t) index] = !selection[(size_t) index];

    bool anySelected = false;
    for (bool s : selection)
        anySelected |= s;
    if (!anySelected)
        selection[(size_t) index] = true;  // always keep at least one selected

    instanceButtons[(size_t) index]->setToggleState(selection[(size_t) index], juce::dontSendNotification);
    for (auto* c : controls)
        c->refreshFromSelection();
}

void MultiInstanceSection::paint(juce::Graphics& g)
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

void MultiInstanceSection::resized()
{
    auto b = getLocalBounds();
    b.removeFromTop(kHeaderHeight);

    for (int row = 0; row < buttonRows; ++row)
    {
        auto selectRow = b.removeFromTop(kSelectRowHeight).reduced(kPadding, 2);
        const int bw = selectRow.getWidth() / juce::jmax(1, buttonColumns);
        for (int col = 0; col < buttonColumns; ++col)
        {
            const int idx = row * buttonColumns + col;
            if (idx >= numInstances)
                break;
            instanceButtons[(size_t) idx]->setBounds(selectRow.removeFromLeft(bw).reduced(2));
        }
    }

    b.reduce(kPadding, kPadding);
    int x = b.getX();
    int y = b.getY();
    int col = 0;
    for (auto* c : controls)
    {
        c->setBounds(x, y, MultiInstanceParamControl::kWidth, MultiInstanceParamControl::kHeight);
        x += MultiInstanceParamControl::kWidth;
        if (++col >= columns)
        {
            col = 0;
            x = b.getX();
            y += MultiInstanceParamControl::kHeight;
        }
    }
}

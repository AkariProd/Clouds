#include "ParamControl.h"
#include "LookAndFeelP12.h"

ParamControl::ParamControl(juce::AudioProcessorValueTreeState& apvts, const p12::ParamDef& def)
    : apvtsRef(apvts), paramDef(def)
{
    // Strip the common prefix (e.g. "Osc 1 ") for a shorter on-knob caption;
    // the section header already names the group.
    juce::String label(def.name);
    nameLabel.setText(label, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    nameLabel.setMinimumHorizontalScale(0.7f);
    addAndMakeVisible(nameLabel);

    if (def.type == p12::ParamType::Toggle)
    {
        toggle = std::make_unique<juce::ToggleButton>();
        addAndMakeVisible(*toggle);
        buttonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, def.id, *toggle);
    }
    else if (def.type == p12::ParamType::Enum && def.treeCategorized)
    {
        // A flat 28-99 item dropdown (Mod Source/Destination, LFO/Envelope
        // Destination) is unusable -- group it into a tree of submenus
        // instead. Not a juce::ComboBox, so no ComboBoxAttachment; wire it
        // by hand and poll (like MultiInstanceParamControl) to pick up
        // changes made from incoming hardware MIDI or host automation.
        treeCombo = std::make_unique<CategorizedCombo>();
        juce::StringArray labels, categories;
        for (int i = 0; i < def.numOptions; ++i)
        {
            labels.add(p12::kOptionStrings[def.firstOptionIndex + i]);
            categories.add(p12::kOptionCategories[def.firstOptionIndex + i]);
        }
        treeCombo->setOptions(labels, categories);
        if (auto* raw = apvts.getRawParameterValue(def.id))
            treeCombo->setSelectedIndex((int) raw->load(), juce::dontSendNotification);
        treeCombo->onChange = [this]
        {
            if (auto* param = apvtsRef.getParameter(paramDef.id))
                param->setValueNotifyingHost(param->convertTo0to1((float) treeCombo->getSelectedIndex()));
        };
        addAndMakeVisible(*treeCombo);
        startTimerHz(15);
    }
    else if (def.type == p12::ParamType::Enum)
    {
        combo = std::make_unique<juce::ComboBox>();
        for (int i = 0; i < def.numOptions; ++i)
            combo->addItem(p12::kOptionStrings[def.firstOptionIndex + i], i + 1);
        if (def.numOptions == 0)
            combo->addItem("Off", 1);
        addAndMakeVisible(*combo);
        comboAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, def.id, *combo);
    }
    else
    {
        slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                 juce::Slider::NoTextBox);
        slider->setRange((double) def.minValue, (double) def.maxValue, 1.0);
        addAndMakeVisible(*slider);
        sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, def.id, *slider);
    }
}

void ParamControl::timerCallback()
{
    if (!treeCombo)
        return;
    if (auto* raw = apvtsRef.getRawParameterValue(paramDef.id))
    {
        const int idx = (int) raw->load();
        if (idx != treeCombo->getSelectedIndex())
            treeCombo->setSelectedIndex(idx, juce::dontSendNotification);
    }
}

void ParamControl::paint(juce::Graphics&) {}

void ParamControl::resized()
{
    auto b = getLocalBounds();
    auto labelArea = b.removeFromBottom(22);
    nameLabel.setBounds(labelArea);

    if (slider)
        slider->setBounds(b.reduced(4));
    else if (toggle)
        toggle->setBounds(b.reduced(4, 10));
    else if (combo)
        combo->setBounds(b.reduced(2).withSizeKeepingCentre(b.getWidth() - 4, 22));
    else if (treeCombo)
        treeCombo->setBounds(b.reduced(2).withSizeKeepingCentre(b.getWidth() - 4, 22));
}

#include "MultiInstanceParamControl.h"
#include "LookAndFeelP12.h"

MultiInstanceParamControl::MultiInstanceParamControl(juce::AudioProcessorValueTreeState& apvtsIn,
                                                       const juce::String& fieldLabel,
                                                       std::vector<const p12::ParamDef*> defsPerInstance,
                                                       const std::vector<bool>& selectionIn)
    : apvts(apvtsIn), defs(std::move(defsPerInstance)), selection(selectionIn)
{
    for (auto* d : defs)
        if (d != nullptr) { representativeDef = d; break; }

    nameLabel.setText(fieldLabel, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    nameLabel.setMinimumHorizontalScale(0.7f);
    addAndMakeVisible(nameLabel);

    if (representativeDef == nullptr)
        return;

    if (representativeDef->type == p12::ParamType::Toggle)
    {
        toggle = std::make_unique<juce::ToggleButton>();
        toggle->onClick = [this] { if (!updatingFromCode) applyToSelected(toggle->getToggleState() ? 1.0f : 0.0f); };
        addAndMakeVisible(*toggle);
    }
    else if (representativeDef->type == p12::ParamType::Enum && representativeDef->treeCategorized)
    {
        treeCombo = std::make_unique<CategorizedCombo>();
        juce::StringArray labels, categories;
        for (int i = 0; i < representativeDef->numOptions; ++i)
        {
            labels.add(p12::kOptionStrings[representativeDef->firstOptionIndex + i]);
            categories.add(p12::kOptionCategories[representativeDef->firstOptionIndex + i]);
        }
        treeCombo->setOptions(labels, categories);
        treeCombo->onChange = [this] { if (!updatingFromCode) applyToSelected((float) treeCombo->getSelectedIndex()); };
        addAndMakeVisible(*treeCombo);
    }
    else if (representativeDef->type == p12::ParamType::Enum)
    {
        combo = std::make_unique<juce::ComboBox>();
        for (int i = 0; i < representativeDef->numOptions; ++i)
            combo->addItem(p12::kOptionStrings[representativeDef->firstOptionIndex + i], i + 1);
        if (representativeDef->numOptions == 0)
            combo->addItem("Off", 1);
        combo->onChange = [this] { if (!updatingFromCode) applyToSelected((float) (combo->getSelectedId() - 1)); };
        addAndMakeVisible(*combo);
    }
    else
    {
        slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        slider->setRange((double) representativeDef->minValue, (double) representativeDef->maxValue, 1.0);
        slider->onValueChange = [this] { if (!updatingFromCode) applyToSelected((float) slider->getValue()); };
        addAndMakeVisible(*slider);
    }

    refreshFromSelection();
    startTimerHz(15);
}

int MultiInstanceParamControl::firstSelectedWithDef() const
{
    for (size_t i = 0; i < defs.size(); ++i)
        if (selection[i] && defs[i] != nullptr)
            return (int) i;
    return -1;
}

void MultiInstanceParamControl::applyToSelected(float realValue)
{
    for (size_t i = 0; i < defs.size(); ++i)
    {
        if (!selection[i] || defs[i] == nullptr)
            continue;
        if (auto* param = apvts.getParameter(defs[i]->id))
            param->setValueNotifyingHost(param->convertTo0to1(realValue));
    }
}

void MultiInstanceParamControl::refreshFromSelection()
{
    if (representativeDef == nullptr)
        return;

    const int idx = firstSelectedWithDef();
    const bool enabled = idx >= 0;

    updatingFromCode = true;
    if (slider)
    {
        slider->setEnabled(enabled);
        if (enabled)
            if (auto* raw = apvts.getRawParameterValue(defs[(size_t) idx]->id))
                slider->setValue((double) raw->load(), juce::dontSendNotification);
    }
    else if (toggle)
    {
        toggle->setEnabled(enabled);
        if (enabled)
            if (auto* raw = apvts.getRawParameterValue(defs[(size_t) idx]->id))
                toggle->setToggleState(raw->load() > 0.5f, juce::dontSendNotification);
    }
    else if (combo)
    {
        combo->setEnabled(enabled);
        if (enabled)
            if (auto* raw = apvts.getRawParameterValue(defs[(size_t) idx]->id))
                combo->setSelectedId((int) raw->load() + 1, juce::dontSendNotification);
    }
    else if (treeCombo)
    {
        treeCombo->setEnabled(enabled);
        if (enabled)
            if (auto* raw = apvts.getRawParameterValue(defs[(size_t) idx]->id))
                treeCombo->setSelectedIndex((int) raw->load(), juce::dontSendNotification);
    }
    updatingFromCode = false;
}

void MultiInstanceParamControl::paint(juce::Graphics&) {}

void MultiInstanceParamControl::resized()
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

#include "MultiOscParamControl.h"
#include "LookAndFeelP12.h"

MultiOscParamControl::MultiOscParamControl(juce::AudioProcessorValueTreeState& apvtsIn,
                                            const juce::String& fieldLabel,
                                            std::array<const p12::ParamDef*, 4> defsPerOsc,
                                            const std::array<bool, 4>& selectionIn)
    : apvts(apvtsIn), defs(defsPerOsc), selection(selectionIn)
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

int MultiOscParamControl::firstSelectedWithDef() const
{
    for (int i = 0; i < 4; ++i)
        if (selection[(size_t) i] && defs[(size_t) i] != nullptr)
            return i;
    return -1;
}

void MultiOscParamControl::applyToSelected(float realValue)
{
    for (int i = 0; i < 4; ++i)
    {
        if (!selection[(size_t) i] || defs[(size_t) i] == nullptr)
            continue;
        if (auto* param = apvts.getParameter(defs[(size_t) i]->id))
            param->setValueNotifyingHost(param->convertTo0to1(realValue));
    }
}

void MultiOscParamControl::refreshFromSelection()
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
    updatingFromCode = false;
}

void MultiOscParamControl::paint(juce::Graphics&) {}

void MultiOscParamControl::resized()
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
}

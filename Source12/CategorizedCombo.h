#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// A combo-box-like control whose popup is a TREE: options are grouped into
// named category submenus (e.g. "Oscillators", "LFOs", "Envelopes") instead
// of one long flat list. Used for the Mod Matrix Source/Destination and
// LFO/Envelope Destination dropdowns, which have 28-99 options each --
// unusable as a flat list, much more findable grouped.
class CategorizedCombo : public juce::Component
{
public:
    CategorizedCombo();

    // categories[i] groups labels[i] into a submenu; an empty string means
    // "no category" -- that item sits directly in the top-level menu.
    void setOptions(juce::StringArray labelsIn, juce::StringArray categoriesIn);

    void setSelectedIndex(int index, juce::NotificationType notify);
    int getSelectedIndex() const { return selectedIndex; }

    // Fired on a user pick (never on a programmatic setSelectedIndex with
    // dontSendNotification), same contract as juce::ComboBox::onChange.
    std::function<void()> onChange;

    void resized() override;
    void enablementChanged() override { button.setEnabled(isEnabled()); }

private:
    void showMenu();

    juce::TextButton button { {}, {} };
    juce::StringArray labels, categories;
    int selectedIndex = -1;
};

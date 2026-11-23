#include "CategorizedCombo.h"
#include <map>

CategorizedCombo::CategorizedCombo()
{
    addAndMakeVisible(button);
    button.onClick = [this] { showMenu(); };
}

void CategorizedCombo::setOptions(juce::StringArray labelsIn, juce::StringArray categoriesIn)
{
    labels = std::move(labelsIn);
    categories = std::move(categoriesIn);
    if (selectedIndex < 0 && labels.size() > 0)
        selectedIndex = 0;
    if (juce::isPositiveAndBelow(selectedIndex, labels.size()))
        button.setButtonText(labels[selectedIndex]);
}

void CategorizedCombo::setSelectedIndex(int index, juce::NotificationType notify)
{
    if (!juce::isPositiveAndBelow(index, labels.size()))
        return;
    selectedIndex = index;
    button.setButtonText(labels[selectedIndex]);
    if (notify == juce::sendNotification && onChange)
        onChange();
}

void CategorizedCombo::resized()
{
    button.setBounds(getLocalBounds());
}

void CategorizedCombo::showMenu()
{
    juce::PopupMenu root;

    // Group indices by category, preserving first-seen category order and
    // each category's own internal item order (both just follow the
    // generated option list order, i.e. the manual's own table order).
    juce::StringArray categoryOrder;
    std::map<juce::String, juce::PopupMenu> submenus;
    std::map<juce::String, int> tickedInCategory;  // category -> selected item id, if any

    for (int i = 0; i < labels.size(); ++i)
    {
        const auto& cat = categories[i];
        if (cat.isEmpty())
        {
            root.addItem(i + 1, labels[i], true, i == selectedIndex);
            continue;
        }
        if (!categoryOrder.contains(cat))
            categoryOrder.add(cat);
        submenus[cat].addItem(i + 1, labels[i], true, i == selectedIndex);
    }

    for (const auto& cat : categoryOrder)
        root.addSubMenu(cat, submenus[cat]);

    root.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&button),
                        [this](int result)
                        {
                            if (result <= 0)
                                return;
                            setSelectedIndex(result - 1, juce::sendNotification);
                        });
}

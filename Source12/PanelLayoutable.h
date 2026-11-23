#pragma once

// Tiny shared interface so the editor's flow-wrap layout can treat
// SectionPanel and OscillatorSection identically without caring which one
// it's placing.
class PanelLayoutable
{
public:
    virtual ~PanelLayoutable() = default;
    virtual int getPreferredWidth() const = 0;
    virtual int getPreferredHeight() const = 0;
};

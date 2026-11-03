#pragma once

#include "algo/Types.h"

#include <juce_gui_basics/juce_gui_basics.h>

// A chord box with 6 strings and 5 frets (T11-04). It draws at widths from 48 to 160 pixels.
class ChordDiagram : public juce::Component
{
public:
    ChordDiagram () = default;

    void setChord (const algo::Voicing& voicing, const juce::String& name);
    void setLeftHanded (bool on);
    void setShowName (bool on);

    void paint (juce::Graphics& g) override;

    // Draws a diagram in an area. The song overview cells use this with no component for each bar.
    static void draw (juce::Graphics& g, juce::Rectangle<float> area, const algo::Voicing& voicing, const juce::String& name, bool leftHanded, bool showName);

    // The base fret: 1 if all frets are 5 or less, or the lowest fretted note.
    static int baseFret (const algo::Voicing& voicing);

private:
    algo::Voicing m_voicing;
    juce::String  m_name;
    bool          m_leftHanded { false };
    bool          m_showName { true };
};

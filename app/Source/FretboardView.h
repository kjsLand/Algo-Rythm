#pragma once

#include "AppState.h"

#include <juce_gui_basics/juce_gui_basics.h>

// The fretboard of the selected bar (T11-06): the voicing notes in the chord colour, the root in the root
// colour, the other scale notes in the scale colour, and the capo bar. The left-handed view mirrors it (T16-07).
class FretboardView : public juce::Component,
                      private juce::ChangeListener
{
public:
    explicit FretboardView (AppState& state);
    ~FretboardView () override;

    void paint (juce::Graphics& g) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    AppState& m_state;
};

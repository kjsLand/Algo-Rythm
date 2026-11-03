#pragma once

#include "AppState.h"

#include <juce_gui_basics/juce_gui_basics.h>

// The strum pattern of the selected section (T11-07): a beat ruler and one symbol for each slot.
// Down, Up, Rest, Mute and Chuck have different symbols. An accent makes the symbol larger.
class StrumView : public juce::Component,
                  private juce::ChangeListener
{
public:
    explicit StrumView (AppState& state);
    ~StrumView () override;

    void paint (juce::Graphics& g) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    AppState& m_state;
};

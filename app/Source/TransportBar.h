#pragma once

#include "AppState.h"
#include "Sequencer.h"

#include <juce_gui_basics/juce_gui_basics.h>

// Play, Stop, Loop Section and the click (T12-04).
class TransportBar : public juce::Component,
                     private juce::ChangeListener,
                     private juce::Timer
{
public:
    TransportBar (AppState& state, Sequencer& sequencer);
    ~TransportBar () override;

    void togglePlay ();
    void setClick (bool on);

    std::function<void (bool)> onClickChanged;

    void paint (juce::Graphics& g) override;
    void resized () override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback () override;
    void update ();

    AppState&          m_state;
    Sequencer&         m_sequencer;
    juce::TextButton   m_play { "Play" }, m_stop { "Stop" };
    juce::ToggleButton m_loop { "Loop section" }, m_click { "Click" };
};

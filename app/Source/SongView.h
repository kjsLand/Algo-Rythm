#pragma once

#include "AppState.h"
#include "LockButton.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

// The song overview (T11-05): one tab for each section and one cell for each bar, with the chord name,
// the Nashville number and a chord diagram (REQ-11-3). Each tab has a drag handle for drag to DAW (T11-08),
// 3 lock buttons (T14-02) and 3 reroll buttons (T14-03).
class SongView : public juce::Component,
                 private juce::ChangeListener,
                 private juce::Timer
{
public:
    explicit SongView (AppState& state);
    ~SongView () override;

    // Returns the playback position in ticks from the start of the song, or -1 when playback stops.
    void setPositionProvider (std::function<double ()> provider);

    // Writes a temporary MIDI file of one section. The drag handle uses it.
    std::function<juce::File (int section)> makeSectionMidi;

    void paint (juce::Graphics& g) override;
    void resized () override;

private:
    class SectionTab;
    class BarGrid;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback () override;
    void rebuild ();

    AppState&                                m_state;
    std::function<double ()>                 m_position;
    std::vector<std::unique_ptr<SectionTab>> m_tabs;
    std::unique_ptr<BarGrid>                 m_grid;

    LockButton       m_lockChords { "chords" }, m_lockVoicings { "voicings" }, m_lockStrum { "strum" };
    juce::Label      m_lockLabel;
    juce::TextButton m_newProgression { "New progression" }, m_newVoicing { "New voicing" }, m_newStrum { "New strum" };

    std::shared_ptr<const algo::SongStarter> m_shown;
};

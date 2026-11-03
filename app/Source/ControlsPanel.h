#pragma once

#include "AppState.h"
#include "LockButton.h"

#include <juce_gui_basics/juce_gui_basics.h>

// The settings controls (T11-03) and their locks (T14-02). A change updates the settings in AppState. It does not generate.
class ControlsPanel : public juce::Component,
                      private juce::ChangeListener
{
public:
    explicit ControlsPanel (AppState& state);
    ~ControlsPanel () override;

    void paint (juce::Graphics& g) override;
    void resized () override;

    // Reads the settings into the controls.
    void refresh ();

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void push ();

    void addLabelled (juce::Component& control, juce::Label& label, const juce::String& text);

    AppState& m_state;

    juce::ComboBox     m_key, m_mood, m_form, m_difficulty, m_capo, m_tuning, m_time;
    juce::Label        m_keyLabel, m_moodLabel, m_formLabel, m_difficultyLabel, m_capoLabel, m_tuningLabel, m_timeLabel;
    juce::Slider       m_bpmMin, m_bpmMax;
    juce::Label        m_bpmMinLabel, m_bpmMaxLabel;
    juce::ToggleButton m_hook { "Hook" };
    juce::TextEditor   m_seed;
    juce::Label        m_seedLabel;
    juce::TextButton   m_copy { "Copy" };
    juce::TextButton   m_generate { "Generate" };

    LockButton m_lockKey { "key" }, m_lockScale { "scale" }, m_lockTempo { "tempo" }, m_lockForm { "form" };

    bool m_updating { false };
};

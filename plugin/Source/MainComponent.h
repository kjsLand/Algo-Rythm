#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Fretboard.h"

class MainComponent : public juce::Component,
                      public juce::KeyListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint    (juce::Graphics& g) override;
    void resized  ()                  override;

    bool keyPressed (const juce::KeyPress& key, juce::Component*) override;

private:
    void pickNext();
    void drawPick (juce::Graphics& g, const GuitarFretboard::ScaleNotePick& pick) const;

    // ---- fretboard state ----
    GuitarFretboard fb;

    const std::string              scaleKey   { "G" };
    const std::string              scaleName  { "Major" };
    const std::vector<std::string> chordTypes {
        "Major", "Minor", "Maj7", "M7", "Dom7",
        "Sus2",  "Sus4",  "Add9", "Dim", "Dim7", "M7b5"
    };

    std::vector<GuitarFretboard::ScaleNotePick> currentPicks;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
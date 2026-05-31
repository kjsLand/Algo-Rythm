#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Fretboard.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint   (juce::Graphics& g) override;
    void resized ()                  override;

private:
    // ---- actions ----
    void pickNewNote();
    void pickNewChord();
    void pickNewKey();

    // ---- drawing ----
    void drawHeader    (juce::Graphics& g, juce::Rectangle<int> r) const;
    void drawFretboard (juce::Graphics& g, juce::Rectangle<int> bounds) const;
    void drawLegend    (juce::Graphics& g, juce::Rectangle<int> bounds) const;

    // ---- fretboard state ----
    GuitarFretboard fb;

    // All available keys (chromatic)
    static const std::vector<std::string> ALL_KEYS;

    std::string currentKey      { "G" };
    std::string currentScale    { "Major" };
    std::string currentChordName{ "Major" };

    GuitarFretboard::Position             rootPos   { 1, 5 }; // A2 fret 5 = D... prime on load
    GuitarFretboard::VoicedChord          voicing;
    std::set<int>                         scalePCs;

    std::mt19937 rng { std::random_device{}() };

    // ---- buttons ----
    juce::TextButton btnNewNote  { "New Note"  };
    juce::TextButton btnNewChord { "New Chord" };
    juce::TextButton btnNewKey   { "New Key"   };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
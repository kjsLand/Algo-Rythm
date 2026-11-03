#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Fretboard.h"
#include "StrumPattern.h"

#include <random>
#include <set>
#include <string>
#include <vector>

class MainComponent : public juce::Component
{
public:
    MainComponent ();
    ~MainComponent () override;

    void paint (juce::Graphics& g) override;
    void resized () override;

private:
    // ---- actions ----
    void pickNewNote ();
    void pickNewChord ();
    void pickNewKey ();
    void pickNewStrum ();

    // ---- drawing ----
    void drawHeader (juce::Graphics& g, juce::Rectangle<int> r) const;
    void drawFretboard (juce::Graphics& g, juce::Rectangle<int> bounds) const;
    void drawLegend (juce::Graphics& g, juce::Rectangle<int> bounds) const;
    void drawStrumPanel (juce::Graphics& g, juce::Rectangle<int> bounds) const;

    // ---- fretboard state ----
    algo::Fretboard fb;

    // All available keys (chromatic)
    static const std::vector<std::string> ALL_KEYS;

    std::string currentKey { "G" };
    std::string currentScale { "Major" };
    std::string currentChordName { "Major" };

    algo::FretPos rootPos { 1, 5 }; // A2 fret 5 = D... prime on load
    VoicedChord   voicing;
    std::set<int> scalePCs;

    std::mt19937 rng { std::random_device {}() };

    // ---- strum pattern ----
    StrumPattern::Result currentStrum;

    // ---- buttons ----
    juce::TextButton btnNewNote { "New Note" };
    juce::TextButton btnNewChord { "New Chord" };
    juce::TextButton btnNewKey { "New Key" };
    juce::TextButton btnNewStrum { "New Strum" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
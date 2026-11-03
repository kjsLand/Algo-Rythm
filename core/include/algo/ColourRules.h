#pragma once

#include <string>
#include <vector>

namespace algo
{
    // The colour chord rules of each difficulty. Beginner has no colour chords.
    struct ColourRules
    {
        // Intermediate: change a chord to a colour chord.
        int                      maxColourChords { 2 }; // For each section
        double                   colourChance { 0.25 }; // For each chord
        std::vector<std::string> colourSymbols { "sus2", "sus4", "add9" };

        // Intermediate: add one chord from outside the scale. These numerals are relative to the major scale.
        double                   borrowedChance { 0.15 }; // For each section
        std::vector<std::string> borrowedNumerals { "b7", "4m", "b6" };

        // Advanced: change a chord to its diatonic 7th chord.
        double           seventhChance { 0.4 }; // For each chord
        std::vector<int> seventhDegrees { 2, 4, 5 };

        // Advanced: add a secondary dominant before the target chord.
        double      secondaryChance { 0.1 }; // For each section
        std::string secondaryNumeral { "37" };
        int         secondaryTarget { 6 };
    };
}

#pragma once

#include "Fretboard.h"
#include "Types.h"

#include <array>

namespace algo
{
    // The lowest note of a voicing has no penalty from E2 to A3.
    constexpr MidiNote kLowestNoteMin = 40;
    constexpr MidiNote kLowestNoteMax = 57;

    // handPosition returns a whole number: the mean of 1 to 6 frets, in these units.
    constexpr int kHandUnitsPerFret = 60;

    // The Coding Spec Section 6.4 was not available, so these values are inferred from the
    // acceptance checks in SPEC-07 and from the voicings that they select. A cost is a whole number, so that a sum of costs and a
    // comparison of two sums give the same result on all systems.
    struct CostWeights
    {
        int barreString { 3 };    // For each string that a barre covers
        int span { 2 };           // For each fret of the fret span
        int finger { 2 };         // For each finger
        int openString { 2 };     // A reward for each open string
        int mutedString { 4 };    // For each muted string, because a strum sounds better on more strings
        int lowestNote { 10 };    // The lowest note is out of the range E2 to A3
        int position { 1 };       // For each fret of the hand position, because high frets are harder to find
        int fingerReach { 8 };    // Two fingers on one fret, more than 2 strings apart
        int beginnerFactor { 4 }; // At Beginner, a barre and the hand position cost this many times more
        int handMove { 3 };       // For each fret that the hand moves
        int commonTone { 1 };     // A reward for each note in both voicings
    };

    // The mean fret of the fretted notes, in kHandUnitsPerFret units. With no fretted note, 0.
    int handPosition (const Voicing& voicing);

    // A barre has a cost only if the hand needs it. With 4 or fewer fretted notes, each note can
    // have its own finger, as in the open A chord. Throws std::out_of_range if a fret is not on the neck.
    int staticCost (const Voicing& voicing, Difficulty difficulty, const Fretboard& fretboard = Fretboard {}, const CostWeights& weights = CostWeights {});

    // The values of a voicing that a transition uses. Sequence selection compares many pairs of voicings.
    struct TransitionPoint
    {
        int                               hand { 0 };
        std::array<MidiNote, kNumStrings> notes {}; // Sorted, with no repeated note
        int                               noteCount { 0 };
    };

    TransitionPoint transitionPoint (const Voicing& voicing, const Fretboard& fretboard = Fretboard {});

    int transitionCost (const TransitionPoint& previous, const TransitionPoint& next, const CostWeights& weights = CostWeights {});
    int transitionCost (const Voicing& previous, const Voicing& next, const Fretboard& fretboard = Fretboard {}, const CostWeights& weights = CostWeights {});
}

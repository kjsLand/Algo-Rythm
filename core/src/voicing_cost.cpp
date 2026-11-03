#include "algo/VoicingCost.h"

#include "algo/Voicing.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>

namespace algo
{
    namespace
    {
        // Counts the notes in two sorted lists.
        int commonNotes (const TransitionPoint& a, const TransitionPoint& b)
        {
            int count = 0;
            for (int i = 0, j = 0; i < a.noteCount && j < b.noteCount;)
            {
                const MidiNote noteA = a.notes[static_cast<std::size_t> (i)];
                const MidiNote noteB = b.notes[static_cast<std::size_t> (j)];

                if (noteA == noteB)
                    ++count;
                if (noteA <= noteB)
                    ++i;
                if (noteB <= noteA)
                    ++j;
            }
            return count;
        }

        // A barre holds its notes with one finger. Other notes on one fret need a finger each,
        // and the hand cannot reach far with two fingers on one fret.
        bool hasFingerReach (const Voicing& voicing)
        {
            const auto barre = findBarre (voicing);

            const auto isFree = [&] (std::size_t s)
            {
                const int  string = static_cast<int> (s);
                const bool onBar  = barre && voicing.frets[s] == barre->fret && string >= barre->firstString && string <= barre->lastString;
                return voicing.frets[s] > 0 && !onBar;
            };

            constexpr std::size_t kMaxReach = 2;

            for (std::size_t low = 0; low < voicing.frets.size (); ++low)
                for (std::size_t high = low + kMaxReach + 1; high < voicing.frets.size (); ++high)
                    if (voicing.frets[low] == voicing.frets[high] && isFree (low) && isFree (high))
                        return true;
            return false;
        }
    }

    int handPosition (const Voicing& voicing)
    {
        int sum   = 0;
        int count = 0;
        for (int fret : voicing.frets)
            if (fret > 0)
            {
                sum += fret;
                ++count;
            }
        return count == 0 ? 0 : sum * kHandUnitsPerFret / count;
    }

    int staticCost (const Voicing& voicing, Difficulty difficulty, const Fretboard& fretboard, const CostWeights& weights)
    {
        int      fretted = 0;
        int      open    = 0;
        int      muted   = 0;
        int      lowFret = 0;
        int      topFret = 0;
        MidiNote lowest  = kLowestNoteMin;
        bool     sounds  = false;

        for (int s = 0; s < kNumStrings; ++s)
        {
            const int fret = voicing.frets[static_cast<std::size_t> (s)];
            if (fret == kMutedString)
            {
                ++muted;
                continue;
            }

            const MidiNote note = fretboard.midiNoteAt (s, fret);
            lowest              = sounds ? std::min (lowest, note) : note;
            sounds              = true;

            if (fret == 0)
            {
                ++open;
                continue;
            }

            ++fretted;
            lowFret = lowFret == 0 ? fret : std::min (lowFret, fret);
            topFret = std::max (topFret, fret);
        }

        const int factor = difficulty == Difficulty::Beginner ? weights.beginnerFactor : 1;

        int cost = (topFret - lowFret) * weights.span + fingerCount (voicing) * weights.finger - open * weights.openString + muted * weights.mutedString;

        cost += weights.position * factor * handPosition (voicing) / kHandUnitsPerFret;

        if (const auto barre = findBarre (voicing); barre && fretted > kMaxFingers)
            cost += (barre->lastString - barre->firstString + 1) * weights.barreString * factor;

        if (hasFingerReach (voicing))
            cost += weights.fingerReach;

        if (lowest < kLowestNoteMin || lowest > kLowestNoteMax)
            cost += weights.lowestNote;

        return cost;
    }

    TransitionPoint transitionPoint (const Voicing& voicing, const Fretboard& fretboard)
    {
        TransitionPoint point;
        point.hand = handPosition (voicing);

        for (int s = 0; s < kNumStrings; ++s)
        {
            const int fret = voicing.frets[static_cast<std::size_t> (s)];
            if (fret != kMutedString)
                point.notes[static_cast<std::size_t> (point.noteCount++)] = fretboard.midiNoteAt (s, fret);
        }

        const auto first = point.notes.begin ();
        const auto last  = first + point.noteCount;
        std::sort (first, last);
        point.noteCount = static_cast<int> (std::unique (first, last) - first);
        return point;
    }

    int transitionCost (const TransitionPoint& previous, const TransitionPoint& next, const CostWeights& weights)
    {
        return weights.handMove * std::abs (previous.hand - next.hand) / kHandUnitsPerFret - weights.commonTone * commonNotes (previous, next);
    }

    int transitionCost (const Voicing& previous, const Voicing& next, const Fretboard& fretboard, const CostWeights& weights)
    {
        return transitionCost (transitionPoint (previous, fretboard), transitionPoint (next, fretboard), weights);
    }
}

#pragma once

#include "Chords.h"
#include "Data.h"
#include "Fretboard.h"
#include "Types.h"
#include "Voicing.h"

#include <array>
#include <map>
#include <tuple>
#include <vector>

namespace algo
{
    // Holds the candidates of each chord, because the fret search is the slowest part of the voicing
    // stage. Use one cache for one DataSet. Do not use a cache from two threads at the same time.
    class VoicingCache
    {
    public:
        // Returns the candidates that pass the filter. A Beginner chord uses only the shapes if a shape
        // passes. Only the first call for a chord, difficulty, tuning and capo makes the candidates.
        // The reference stays valid for the life of the cache.
        const std::vector<Voicing>& candidates (const ConcreteChord& chord, Difficulty difficulty, const Fretboard& fretboard, const DataSet& data = bundledData ());

        // The number of times that a call had to make the candidates.
        int searchCount () const
        {
            return m_searchCount;
        }

    private:
        using Entry = std::tuple<PitchClass, std::string, int, Difficulty, std::array<MidiNote, kNumStrings>, int>;

        std::map<Entry, std::vector<Voicing>> m_candidates;
        int                                   m_searchCount { 0 };
    };

    // Puts a voicing on each chord of the section. A section that repeats its chords gets one sequence
    // for one pass, so a chord position always has the same voicing and a reroll changes all its bars.
    // A reroll count selects one of the best sequences. It starts again after the last sequence.
    // Throws GenerationError if a chord has no candidate, and std::invalid_argument if the count is negative.
    SectionChords voiceSection (const SectionChords& section, const Key& key, const Fretboard& fretboard, Difficulty difficulty, int rerollCount, VoicingCache& cache, const DataSet& data = bundledData (), const CostWeights& weights = CostWeights {});

    // rerollCounts[i] is the reroll count of section i. A section with no count uses 0.
    ProgressionSet voiceChords (const ProgressionSet& progressions, const Key& key, const Fretboard& fretboard, Difficulty difficulty, const std::vector<int>& rerollCounts, VoicingCache& cache, const DataSet& data = bundledData (), const CostWeights& weights = CostWeights {});
}

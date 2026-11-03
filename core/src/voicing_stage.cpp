#include "algo/Diatonic.h"
#include "algo/GenerationError.h"
#include "algo/Nashville.h"
#include "algo/Pitch.h"
#include "algo/Scale.h"
#include "algo/VoicingCache.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace algo
{
    namespace
    {
        // Sequence selection compares each pair of candidates, so it uses only the cheapest candidates.
        constexpr std::size_t kMaxStageCandidates = 64;

        // Sequence selection does not see the move from the last chord back to the first chord.
        // The stage adds that cost to this number of sequences and keeps the best.
        constexpr int kSequencePool = 25;

        std::vector<Voicing> cheapest (const std::vector<Voicing>& candidates, Difficulty difficulty, const Fretboard& fretboard, const CostWeights& weights)
        {
            struct Ranked
            {
                int         cost;
                std::size_t index;
            };

            std::vector<Ranked> ranked;
            ranked.reserve (candidates.size ());
            for (std::size_t i = 0; i < candidates.size (); ++i)
                ranked.push_back ({ staticCost (candidates[i], difficulty, fretboard, weights), i });

            std::sort (ranked.begin (), ranked.end (), [] (const Ranked& a, const Ranked& b)
            {
                return a.cost != b.cost ? a.cost < b.cost : a.index < b.index;
            });
            if (ranked.size () > kMaxStageCandidates)
                ranked.resize (kMaxStageCandidates);

            std::vector<Voicing> kept;
            kept.reserve (ranked.size ());
            for (const auto& item : ranked)
                kept.push_back (candidates[item.index]);
            return kept;
        }

        // Returns the shortest length that gives all of the chords by repetition.
        std::size_t repeatLength (const std::vector<ChordSymbol>& chords)
        {
            for (std::size_t length = 1; length < chords.size (); ++length)
            {
                bool repeats = true;
                for (std::size_t i = length; repeats && i < chords.size (); ++i)
                    repeats = chords[i] == chords[i % length];

                if (repeats)
                    return length;
            }
            return chords.size ();
        }

        void addLoopClosure (std::vector<VoicingSequence>& sequences, const Fretboard& fretboard, const CostWeights& weights)
        {
            for (auto& sequence : sequences)
                if (sequence.voicings.size () > 1)
                    sequence.cost += transitionCost (sequence.voicings.back (), sequence.voicings.front (), fretboard, weights);

            std::stable_sort (sequences.begin (), sequences.end (), [] (const VoicingSequence& a, const VoicingSequence& b)
            {
                return a.cost < b.cost;
            });
            if (sequences.size () > static_cast<std::size_t> (kBestSequences))
                sequences.resize (static_cast<std::size_t> (kBestSequences));
        }
    }

    ConcreteChord concreteChord (const ChordSymbol& symbol, const Key& key, const DataSet& data)
    {
        ConcreteChord chord;
        chord.root       = chordRootPc (symbol, key, data);
        chord.definition = data.chords.getBySymbol (resolve (symbol, key, data).quality);
        chord.name       = chordName (symbol, key, data);
        if (symbol.bassDegree != 0)
            chord.bass = pitchClassOf (degreePitchClass (key, symbol.bassDegree, data) + symbol.bassAccidental);
        return chord;
    }

    const std::vector<Voicing>& VoicingCache::candidates (const ConcreteChord& chord, Difficulty difficulty, const Fretboard& fretboard, const DataSet& data)
    {
        const Entry entry { chord.root, chord.definition.name, chord.bass ? *chord.bass : -1, difficulty, fretboard.tuning ().strings, fretboard.capo () };

        const auto cached = m_candidates.find (entry);
        if (cached != m_candidates.end ())
            return cached->second;

        ++m_searchCount;

        const auto passes = [&] (const Voicing& voicing)
        {
            return violations (voicing, chord, difficulty, fretboard).empty ();
        };

        std::vector<Voicing> kept;
        for (const auto& voicing : shapeCandidates (chord, fretboard, data))
            if (passes (voicing))
                kept.push_back (voicing);

        // The search finds voicings that pass the filter but that a beginner cannot easily play, such
        // as x20402. A Beginner chord uses the shapes, and uses the search only if no shape passes.
        if (difficulty != Difficulty::Beginner || kept.empty ())
            for (const auto& voicing : searchCandidates (chord, fretboard, difficulty))
                if (passes (voicing))
                    kept.push_back (voicing);

        std::sort (kept.begin (), kept.end (), [] (const Voicing& a, const Voicing& b)
        {
            return a.frets < b.frets;
        });
        kept.erase (std::unique (kept.begin (), kept.end (), [] (const Voicing& a, const Voicing& b)
        {
            return a.frets == b.frets;
        }),
                    kept.end ());

        return m_candidates.emplace (entry, std::move (kept)).first->second;
    }

    SectionChords voiceSection (const SectionChords& section, const Key& key, const Fretboard& fretboard, Difficulty difficulty, int rerollCount, VoicingCache& cache, const DataSet& data, const CostWeights& weights)
    {
        if (rerollCount < 0)
            throw std::invalid_argument ("The reroll count must not be negative");

        std::vector<ChordSymbol> symbols;
        for (const auto& bar : section.bars)
            symbols.insert (symbols.end (), bar.chords.begin (), bar.chords.end ());

        const std::size_t length = repeatLength (symbols);

        std::vector<ConcreteChord>        chords;
        std::vector<std::vector<Voicing>> candidates;
        for (std::size_t i = 0; i < length; ++i)
        {
            chords.push_back (concreteChord (symbols[i], key, data));
            candidates.push_back (cheapest (cache.candidates (chords.back (), difficulty, fretboard, data), difficulty, fretboard, weights));
        }

        auto sequences = chooseSequence (chords, candidates, difficulty, fretboard, weights, kSequencePool);
        addLoopClosure (sequences, fretboard, weights);

        SectionChords voiced = section;
        if (sequences.empty ())
            return voiced;

        const auto& sequence = sequences[static_cast<std::size_t> (rerollCount) % sequences.size ()];

        std::vector<Voicing> fingered;
        for (const auto& voicing : sequence.voicings)
        {
            const auto withFingers = assignFingers (voicing);
            if (!withFingers)
                throw GenerationError ("A voicing that passed the filter needs a fifth finger");
            fingered.push_back (*withFingers);
        }

        std::size_t next = 0;
        for (auto& bar : voiced.bars)
        {
            bar.voicings.clear ();
            for (std::size_t i = 0; i < bar.chords.size (); ++i)
                bar.voicings.push_back (fingered[next++ % length]);
        }
        return voiced;
    }

    ProgressionSet voiceChords (const ProgressionSet& progressions, const Key& key, const Fretboard& fretboard, Difficulty difficulty, const std::vector<int>& rerollCounts, VoicingCache& cache, const DataSet& data, const CostWeights& weights)
    {
        ProgressionSet voiced;
        voiced.sections.reserve (progressions.sections.size ());

        for (std::size_t i = 0; i < progressions.sections.size (); ++i)
        {
            const int rerollCount = i < rerollCounts.size () ? rerollCounts[i] : 0;
            voiced.sections.push_back (voiceSection (progressions.sections[i], key, fretboard, difficulty, rerollCount, cache, data, weights));
        }
        return voiced;
    }
}

#include "algo/Chords.h"

#include "algo/DataError.h"
#include "algo/Diatonic.h"
#include "algo/Nashville.h"
#include "algo/Pitch.h"
#include "algo/Scale.h"

#include "data_reader.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace algo
{
    namespace
    {
        constexpr const char* kBorrowedTag  = "borrowed";
        constexpr const char* kSecondaryTag = "secondary";

        constexpr int kFlatSevenInterval = 10; // The semitones from the tonic to a b7 chord
        constexpr int kFifthDownInterval = 5;  // The semitones from a dominant up to its target
        constexpr int kSlowBpm           = 80;

        std::size_t indexOf (int value)
        {
            return static_cast<std::size_t> (value);
        }

        template<typename T>
        const T& pick (const std::vector<T>& items, Rng& rng)
        {
            return items[indexOf (rng.uniformInt (0, static_cast<int> (items.size ()) - 1))];
        }

        bool isDegree (const ChordSymbol& chord, int degree)
        {
            return chord.degree == degree && chord.accidental == 0;
        }

        bool hasTag (const ChordSymbol& chord, const char* tag)
        {
            return std::find (chord.tags.begin (), chord.tags.end (), tag) != chord.tags.end ();
        }

        bool hasAnyTag (const ChordLoop& loop, const char* tag)
        {
            return std::any_of (loop.begin (), loop.end (), [tag] (const ChordSymbol& chord)
            {
                return hasTag (chord, tag);
            });
        }

        bool sameRoot (const ChordSymbol& a, const ChordSymbol& b)
        {
            return a.degree == b.degree && a.accidental == b.accidental;
        }

        // A colour chord does not change a progression, so this compares only the roots.
        bool sameRoots (const ChordLoop& a, const ChordLoop& b)
        {
            return std::equal (a.begin (), a.end (), b.begin (), b.end (), sameRoot);
        }

        bool isMajorOrMinorTriad (const ChordSymbol& chord, const Key& key, const DataSet& data)
        {
            const auto quality = resolve (chord, key, data).quality;
            return quality == data.chords.get ("Major").symbol || quality == data.chords.get ("Minor").symbol;
        }

        // A chord that the colour rules can change.
        bool isPlainTriad (const ChordSymbol& chord, const Key& key, const DataSet& data)
        {
            return chord.tags.empty () && chord.bassDegree == 0 && isMajorOrMinorTriad (chord, key, data);
        }

        bool leadsBack (const ChordSymbol& last, const ChordSymbol& first, const Key& key, const DataSet& data)
        {
            const bool fromFour = isDegree (last, 4);
            const bool fromFive = isDegree (last, 5);

            // The numeral "7" is a b7 chord in a scale with a flat 7th, so this measures the interval.
            const bool fromFlatSeven = pitchClassOf (chordRootPc (last, key, data) - key.root) == kFlatSevenInterval;

            if (isDegree (first, 1))
                return fromFive || fromFour || fromFlatSeven;
            if (isDegree (first, 6))
                return fromFour || fromFive;
            return false;
        }

        NumeralMask usableNumerals (const SongPlan& plan, const DataSet& data)
        {
            NumeralMask mask;
            for (int numeral = 1; numeral <= kDiatonicNumerals; ++numeral)
            {
                ChordSymbol chord;
                chord.degree = numeral;

                mask[indexOf (numeral - 1)] = plan.difficulty != Difficulty::Beginner || isMajorOrMinorTriad (chord, plan.key, data);
            }
            return mask;
        }

        // The filters from the first choice to the last choice.
        std::vector<ProgressionFilter> libraryFilters (const SongPlan& plan, const std::string& role, Difficulty difficulty, const DataSet& data)
        {
            const std::string family = isMinorScale (plan.key.scale, data) ? "Natural Minor" : "Major";

            return { { plan.key.scale, role, "", difficulty },
                     { plan.key.scale, "", "", difficulty },
                     { family, role, "", difficulty },
                     { family, "", "", difficulty } };
        }

        // A middle chord can change, because the first and last chords control the loop rules.
        std::vector<int> middleSlots (const ChordLoop& loop, const ChordSymbol& replacement)
        {
            std::vector<int> slots;
            for (std::size_t i = 1; i + 1 < loop.size (); ++i)
                if (loop[i].tags.empty () && !sameRoot (loop[i - 1], replacement) && !sameRoot (loop[i + 1], replacement))
                    slots.push_back (static_cast<int> (i));
            return slots;
        }

        void addColourChords (ChordLoop& loop, const SongPlan& plan, Rng& rng, const DataSet& data, const ColourRules& rules)
        {
            int changed = 0;
            for (auto& chord : loop)
            {
                if (changed == rules.maxColourChords)
                    return;
                if (!isPlainTriad (chord, plan.key, data) || !rng.chance (rules.colourChance))
                    continue;

                // A colour chord must stay in the scale, because it has no tag.
                std::vector<std::string> symbols;
                for (const auto& symbol : rules.colourSymbols)
                {
                    ChordSymbol candidate = chord;
                    candidate.quality     = symbol;
                    if (isDiatonic (candidate, plan.key, data))
                        symbols.push_back (symbol);
                }

                if (!symbols.empty ())
                {
                    chord.quality = pick (symbols, rng);
                    ++changed;
                }
            }
        }

        void addBorrowedChord (ChordLoop& loop, const SongPlan& plan, Rng& rng, const DataSet& data, const ColourRules& rules)
        {
            if (!rng.chance (rules.borrowedChance) || hasAnyTag (loop, kBorrowedTag))
                return;

            // The numerals are relative to the major scale. In a key where a numeral has a different
            // root, or is already diatonic, it is not a borrowed chord.
            const Key                major { plan.key.root, "Major" };
            std::vector<ChordSymbol> candidates;
            for (const auto& numeral : rules.borrowedNumerals)
            {
                ChordSymbol candidate = parseNumeral (numeral);
                candidate.tags.push_back (kBorrowedTag);
                if (chordRootPc (candidate, plan.key, data) == chordRootPc (candidate, major, data) && !isDiatonic (candidate, plan.key, data))
                    candidates.push_back (std::move (candidate));
            }
            if (candidates.empty ())
                return;

            const auto& borrowed = pick (candidates, rng);
            const auto  slots    = middleSlots (loop, borrowed);
            if (!slots.empty ())
                loop[indexOf (pick (slots, rng))] = borrowed;
        }

        void addSevenths (ChordLoop& loop, const SongPlan& plan, Rng& rng, const DataSet& data, const ColourRules& rules)
        {
            for (auto& chord : loop)
            {
                const bool isTarget = chord.accidental == 0 && std::find (rules.seventhDegrees.begin (), rules.seventhDegrees.end (), chord.degree) != rules.seventhDegrees.end ();
                if (isTarget && isPlainTriad (chord, plan.key, data) && rng.chance (rules.seventhChance))
                    chord.quality = diatonicSeventh (plan.key, chord.degree, data).chord.symbol;
            }
        }

        void addSecondaryDominant (ChordLoop& loop, const SongPlan& plan, Rng& rng, const DataSet& data, const ColourRules& rules)
        {
            if (!rng.chance (rules.secondaryChance) || hasAnyTag (loop, kSecondaryTag))
                return;

            ChordSymbol secondary = parseNumeral (rules.secondaryNumeral);
            secondary.tags.push_back (kSecondaryTag);

            // In a scale such as Dorian, the numeral is not a dominant of its target.
            ChordSymbol target;
            target.degree = rules.secondaryTarget;
            if (pitchClassOf (chordRootPc (target, plan.key, data) - chordRootPc (secondary, plan.key, data)) != kFifthDownInterval)
                return;

            std::vector<int> slots;
            for (int slot : middleSlots (loop, secondary))
                if (isDegree (loop[indexOf (slot + 1)], rules.secondaryTarget))
                    slots.push_back (slot);

            if (!slots.empty ())
                loop[indexOf (pick (slots, rng))] = secondary;
        }

        std::vector<Bar> fillBars (const ChordLoop& loop, int barCount, int perBar)
        {
            std::vector<Bar> bars (indexOf (barCount));
            std::size_t      next = 0;
            for (auto& bar : bars)
                for (int i = 0; i < perBar; ++i)
                    bar.chords.push_back (loop[next++ % loop.size ()]);
            return bars;
        }
    }

    ProgressionDefinition pickLibraryLoop (const SongPlan& plan, const std::string& role, Difficulty difficulty, Rng& rng, const DataSet& data)
    {
        for (const auto& filter : libraryFilters (plan, role, difficulty, data))
        {
            const auto progressions = data.progressions.filter (filter);
            if (!progressions.empty ())
                return pick (progressions, rng);
        }
        throw DataError ("progressions.json", "", "no progression has the mode '" + plan.key.scale + "' at this difficulty");
    }

    ChordLoop libraryLoop (const ProgressionDefinition& progression, const Key& key, const DataSet& data)
    {
        const bool  sameMode = detail::toLower (progression.mode) == detail::toLower (key.scale);
        const auto& tags     = progression.tags;
        const char* tag      = std::find (tags.begin (), tags.end (), kSecondaryTag) != tags.end () ? kSecondaryTag : kBorrowedTag;

        ChordLoop loop;
        for (const auto& numeral : progression.numerals)
        {
            ChordSymbol chord = parseNumeral (numeral);

            // A progression from another mode keeps its chord types.
            if (!sameMode)
                chord = resolve (chord, Key { key.root, progression.mode }, data);

            if (!isDiatonic (chord, key, data))
                chord.tags.push_back (tag);

            loop.push_back (std::move (chord));
        }
        return loop;
    }

    std::vector<int> startNumerals (const std::string& role)
    {
        if (role == "verse" || role == "loop")
            return { 1 };
        if (role == "chorus")
            return { 4, 6, 1 };
        if (role == "bridge")
            return { 4, 6, 2 };
        throw std::invalid_argument ("Unknown section role: " + role);
    }

    ChordLoop walkLoop (const SongPlan& plan, const std::string& role, int startNumeral, Rng& rng, const DataSet& data)
    {
        const auto starts = startNumerals (role);
        if (std::find (starts.begin (), starts.end (), startNumeral) == starts.end ())
            throw std::invalid_argument ("A " + role + " cannot start on " + std::to_string (startNumeral));

        const auto mask = usableNumerals (plan, data);
        const auto mode = isMinorScale (plan.key.scale, data) ? TransitionMode::Minor : TransitionMode::Major;

        if (!mask[indexOf (startNumeral - 1)])
            return {};

        ChordLoop loop (1);
        loop[0].degree = startNumeral;

        while (static_cast<int> (loop.size ()) < kLoopLength)
        {
            const auto next = data.transitions.next (mode, loop.back ().degree, mask, rng);
            if (!next)
                return {};

            ChordSymbol chord;
            chord.degree = *next;
            loop.push_back (std::move (chord));
        }
        return loop;
    }

    std::vector<std::string> checkLoop (const ChordLoop& loop, const std::string& role, const std::vector<ChordLoop>& otherLoops, const SongPlan& plan, const DataSet& data)
    {
        if (static_cast<int> (loop.size ()) != kLoopLength)
            return { "The loop must have 4 chords: " + std::to_string (loop.size ()) };

        std::vector<std::string> failures;

        if (!leadsBack (loop.back (), loop.front (), plan.key, data))
            failures.push_back ("The last chord does not lead back to the first chord: " + numeralText (loop.back ()) + " to " + numeralText (loop.front ()));

        if (role == "bridge" && !isDegree (loop.back (), 5))
            failures.push_back ("A bridge must end on 5: " + numeralText (loop.back ()));

        if (role == "chorus")
            for (const auto& other : otherLoops)
                if (sameRoots (loop, other))
                    failures.push_back ("The chorus must not be the same as the verse");

        for (const auto& chord : loop)
        {
            if (!hasTag (chord, kBorrowedTag) && !hasTag (chord, kSecondaryTag) && !isDiatonic (chord, plan.key, data))
                failures.push_back ("The chord is not diatonic and has no tag: " + numeralText (chord));

            if (plan.difficulty == Difficulty::Beginner && !isMajorOrMinorTriad (chord, plan.key, data))
                failures.push_back ("A Beginner chord must be a major or minor triad: " + numeralText (chord));
        }

        return failures;
    }

    ChordLoop makeLoop (const SongPlan& plan, const std::string& role, const std::vector<ChordLoop>& otherLoops, Rng& rng, const DataSet& data, const LoopRules& rules)
    {
        return makeLoopWithCheck (plan, role, [&] (const ChordLoop& loop)
        {
            return checkLoop (loop, role, otherLoops, plan, data);
        }, rng, data, rules);
    }

    ChordLoop makeLoopWithCheck (const SongPlan& plan, const std::string& role, const LoopCheck& check, Rng& rng, const DataSet& data, const LoopRules& rules)
    {
        const auto starts = startNumerals (role);

        for (int attempt = 0; attempt < rules.maxAttempts; ++attempt)
        {
            const auto loop = rng.chance (rules.libraryChance)
                                  ? libraryLoop (pickLibraryLoop (plan, role, plan.difficulty, rng, data), plan.key, data)
                                  : walkLoop (plan, role, pick (starts, rng), rng, data);

            if (check (loop).empty ())
                return loop;
        }

        for (const auto& filter : libraryFilters (plan, role, plan.difficulty, data))
        {
            auto progressions = data.progressions.filter (filter);
            rng.shuffle (progressions);

            for (const auto& progression : progressions)
            {
                const auto loop = libraryLoop (progression, plan.key, data);
                if (check (loop).empty ())
                    return loop;
            }
        }
        throw DataError ("progressions.json", "", "no progression passes the rules for the role '" + role + "'");
    }

    ChordLoop applyColour (const ChordLoop& loop, const SongPlan& plan, Rng& rng, const DataSet& data, const ColourRules& rules)
    {
        ChordLoop coloured = loop;

        if (plan.difficulty == Difficulty::Intermediate)
        {
            addColourChords (coloured, plan, rng, data, rules);
            addBorrowedChord (coloured, plan, rng, data, rules);
        }
        else if (plan.difficulty == Difficulty::Advanced)
        {
            addSevenths (coloured, plan, rng, data, rules);
            addSecondaryDominant (coloured, plan, rng, data, rules);
        }
        return coloured;
    }

    int chordsPerBar (const SongPlan& plan)
    {
        return plan.difficulty == Difficulty::Advanced || plan.bpm < kSlowBpm ? 2 : 1;
    }

    ProgressionSet makeChords (const SongPlan& plan, const RngFactory& rngFactory, const DataSet& data)
    {
        ProgressionSet         result;
        std::vector<ChordLoop> verses;

        for (std::size_t i = 0; i < plan.sections.size (); ++i)
        {
            const auto& section = plan.sections[i];
            Rng         rng     = rngFactory (Stage::Chords, static_cast<int> (i));

            const std::vector<ChordLoop> others = section.role == "chorus" ? verses : std::vector<ChordLoop> {};

            const auto base     = makeLoop (plan, section.role, others, rng, data);
            const auto coloured = applyColour (base, plan, rng, data);

            // A new chord can give the chorus the same roots as the verse. The base loop has passed.
            const auto& loop = checkLoop (coloured, section.role, others, plan, data).empty () ? coloured : base;

            if (section.role == "verse")
                verses.push_back (loop);

            result.sections.push_back ({ section.name, section.role, loop, fillBars (loop, section.bars, chordsPerBar (plan)) });
        }
        return result;
    }
}

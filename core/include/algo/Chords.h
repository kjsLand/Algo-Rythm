#pragma once

#include "ColourRules.h"
#include "Data.h"
#include "Plan.h"
#include "Rng.h"
#include "Seeds.h"
#include "Types.h"

#include <functional>
#include <string>
#include <vector>

namespace algo
{
    constexpr int kLoopLength = 4;

    // A chord can keep the quality "diatonic". Call resolve to get its chord symbol.
    using ChordLoop = std::vector<ChordSymbol>;

    struct LoopRules
    {
        double libraryChance { 0.5 }; // The probability that an attempt uses the library and not the walk
        int    maxAttempts { 50 };
    };

    // The chords of one section. The voicings of each bar stay empty until the voicing stage.
    struct SectionChords
    {
        std::string      name;
        std::string      role;
        ChordLoop        loop;
        std::vector<Bar> bars;
    };

    // The result of stage 2.
    struct ProgressionSet
    {
        std::vector<SectionChords> sections;
    };

    // Filters the library by the mode of the plan, the role and the difficulty. If no progression
    // passes, removes the role filter. A scale with no progressions in the library then uses the
    // progressions of Major or Natural Minor. Throws DataError if no progression is left.
    ProgressionDefinition pickLibraryLoop (const SongPlan& plan, const std::string& role, Difficulty difficulty, Rng& rng, const DataSet& data = bundledData ());

    // Converts a library progression to the chords of the key. A chord that is not diatonic in
    // the key gets the tag "secondary" if the progression has that tag, and if not, "borrowed".
    ChordLoop libraryLoop (const ProgressionDefinition& progression, const Key& key, const DataSet& data = bundledData ());

    // Returns the numerals that a walk can start on. Throws std::invalid_argument if the role is unknown.
    std::vector<int> startNumerals (const std::string& role);

    // Chooses the next numerals from the transition table, with no direct repeats. A Beginner walk
    // uses only major and minor triads. If a Beginner walk has no chord to use, the result is empty.
    // Throws std::invalid_argument if the numeral is not in startNumerals (role).
    ChordLoop walkLoop (const SongPlan& plan, const std::string& role, int startNumeral, Rng& rng, const DataSet& data = bundledData ());

    // Returns one text for each rule that the loop breaks. An empty list means that the loop passes.
    //   - The loop has 4 chords.
    //   - The last chord leads back to the first chord: 5 to 1, 4 to 1, b7 to 1, 4 to 6 or 5 to 6.
    //   - A bridge ends on 5.
    //   - A chorus does not have the same roots as one of otherLoops. Pass the verse loops.
    //   - A chord that is not diatonic has the tag "borrowed" or "secondary".
    //   - A Beginner loop has only major and minor triads.
    std::vector<std::string> checkLoop (const ChordLoop& loop, const std::string& role, const std::vector<ChordLoop>& otherLoops, const SongPlan& plan, const DataSet& data = bundledData ());

    // Returns the rule failures of a loop.
    using LoopCheck = std::function<std::vector<std::string> (const ChordLoop&)>;

    // Makes attempts from the library and the walk until a loop passes. After the last attempt, uses
    // a library progression that passes. Throws DataError if no library progression passes.
    ChordLoop makeLoop (const SongPlan& plan, const std::string& role, const std::vector<ChordLoop>& otherLoops, Rng& rng, const DataSet& data = bundledData (), const LoopRules& rules = LoopRules {});

    // As makeLoop, with a different check.
    ChordLoop makeLoopWithCheck (const SongPlan& plan, const std::string& role, const LoopCheck& check, Rng& rng, const DataSet& data = bundledData (), const LoopRules& rules = LoopRules {});

    // Applies the rules for the difficulty of the plan. The first and last chords keep their roots,
    // so the loop still leads back. A chord from the rules that is not in the scale gets a tag.
    ChordLoop applyColour (const ChordLoop& loop, const SongPlan& plan, Rng& rng, const DataSet& data = bundledData (), const ColourRules& rules = ColourRules {});

    // Returns the number of chords in each bar of the plan.
    int chordsPerBar (const SongPlan& plan);

    // Makes one loop for each section and fills the bars of the section with it. Each section uses its
    // own stream of Stage::Chords. A chorus depends on the verse, but no section depends on a later section.
    ProgressionSet makeChords (const SongPlan& plan, const RngFactory& rngFactory, const DataSet& data = bundledData ());
}

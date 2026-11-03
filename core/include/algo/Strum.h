#pragma once

#include "Data.h"
#include "Plan.h"
#include "Rng.h"
#include "Seeds.h"
#include "StrumLibrary.h"
#include "Types.h"

#include <string>
#include <utility>
#include <vector>

namespace algo
{
    // Above this BPM, the grid has 8 steps (REQ-08-3).
    constexpr int kFastStrumBpm = 140;

    // makeStrums gets the base hit count of the song from this section index of the Strum stream.
    // No section has this index, so a reroll of a section cannot change the base hit count.
    constexpr int kSongStrumSection = -1;

    // A strum on the top strings only. Beginner strums always use 6 strings.
    constexpr int kPartialStrumStrings = 4;

    constexpr double kBackbeatAccent = 1.0; // Beats 2 and 4
    constexpr double kHitAccent      = 0.6; // The other hits

    struct StrumRules
    {
        double libraryChance { 0.5 }; // The probability that a section tries the library first
        double chuckChance { 0.3 };   // At Advanced, for each rest on beat 2 or beat 4
        double muteChance { 0.15 };   // At Advanced, for each hit on an odd slot
    };

    // 8 steps at Beginner or above 140 BPM, and 16 steps for the other levels.
    // A 6/8 time signature has 12 steps (T16-06).
    int gridSteps (Difficulty difficulty, int bpm, const TimeSig& timeSig = TimeSig {});

    // The slots that start a beat. 4/4 has 4 beats and 6/8 has 2 beats (slots 0 and 6).
    std::vector<int> beatSlots (int steps, const TimeSig& timeSig = TimeSig {});

    // The number of slots that sound the strings: Down, Up and Mute.
    int hitCount (const StrumPattern& pattern);

    // True for Rest and Chuck. A Chuck hits the strings with the hand, so no note sounds.
    bool isSilent (Stroke stroke);

    // The range of the base hit count for a grid, so that a chorus can add 2 hits.
    std::pair<int, int> baseHitRange (int steps);

    int chooseBaseHits (int steps, Rng& rng);

    // The two-layer method: a Euclidean strum layer and a Euclidean rest mask.
    // Slot 0 is a Down stroke. At Beginner, slot steps/2 is not a rest. The result has targetHits hits,
    // limited to the range 1 to steps. Throws std::invalid_argument if steps is less than 2.
    StrumPattern euclidStrum (int steps, int targetHits, Difficulty difficulty, Rng& rng, const TimeSig& timeSig = TimeSig {});

    // Sets the accent of beats 2 and 4 to 1.0, of other hits to 0.6 and of silent slots to 0.
    // In 6/8, the strong beat on slot 6 has the accent 1.0.
    void setAccents (StrumPattern& pattern, const TimeSig& timeSig = TimeSig {});

    // At Advanced, changes rests on beats 2 and 4 to Chuck and hits on odd slots to Mute. Does not change slot 0.
    void addPercussion (StrumPattern& pattern, Difficulty difficulty, Rng& rng, const StrumRules& rules = StrumRules {}, const TimeSig& timeSig = TimeSig {});

    // Makes the pattern of one section from its own stream.
    StrumPattern makeSectionStrum (const SongPlan& plan, const std::string& role, int baseHits, Rng& rng, const StrumLibrary& library, const StrumRules& rules = StrumRules {});

    // Returns one pattern for each section of the plan. Each section uses its own stream of Stage::Strum.
    StrumPattern makeSectionStrum (const SongPlan& plan, int sectionIndex, const RngFactory& rngFactory, const StrumLibrary& library, const StrumRules& rules = StrumRules {});

    std::vector<StrumPattern> makeStrums (const SongPlan& plan, const RngFactory& rngFactory, const StrumLibrary& library, const StrumRules& rules = StrumRules {});
    std::vector<StrumPattern> makeStrums (const SongPlan& plan, const RngFactory& rngFactory, const DataSet& data = bundledData ());

    // The base hit count of the song. It uses the stream of kSongStrumSection.
    int songBaseHits (const SongPlan& plan, const RngFactory& rngFactory);

    // Text such as "D - D U - U D U".
    std::string strumText (const StrumPattern& pattern);

    // A beat ruler such as "1 + 2 + 3 + 4 +" for 8 steps, with one character for each slot and a space between.
    std::string strumRuler (const StrumPattern& pattern, const TimeSig& timeSig = TimeSig {});
}

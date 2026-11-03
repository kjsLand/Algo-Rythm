#include "algo/Chords.h"
#include "algo/Plan.h"
#include "algo/VoicingCache.h"

#include "support/VoicingHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <limits>
#include <string>

using namespace algo;

// A speed limit has a meaning only in an optimized build.
#ifdef NDEBUG

namespace
{
    // Returns the best time of some runs, because other work on a CI machine can slow one run.
    template<typename Fn>
    double bestMilliseconds (Fn&& fn, int runs = 5)
    {
        double best = std::numeric_limits<double>::max ();
        for (int i = 0; i < runs; ++i)
        {
            const auto start = std::chrono::steady_clock::now ();
            fn ();
            const std::chrono::duration<double, std::milli> taken = std::chrono::steady_clock::now () - start;
            best                                                  = std::min (best, taken.count ());
        }
        return best;
    }

    // 8 sections of 8 bars. An Advanced plan has 2 chords in each bar.
    SongPlan longPlan (PitchClass root)
    {
        SongPlan plan;
        plan.key        = { root, "Major" };
        plan.difficulty = Difficulty::Advanced;

        for (int i = 0; i < 4; ++i)
        {
            plan.sections.push_back ({ "Verse " + std::to_string (i), "verse", 8 });
            plan.sections.push_back ({ "Chorus " + std::to_string (i), "chorus", 8 });
        }
        return plan;
    }
}

TEST_CASE ("REQ-07-8: the voicings of a 64-bar Advanced song starter take 100 ms or less")
{
    for (PitchClass root : { 0, 6, 10 })
    {
        const auto plan         = longPlan (root);
        const auto progressions = makeChords (plan, [] (Stage stage, int section)
        {
            return rngFor (7, stage, section, 0);
        });

        // A new cache each time: this is the slowest case, with no result from an earlier call.
        const double taken = bestMilliseconds ([&]
        {
            VoicingCache cache;
            REQUIRE (voiceChords (progressions, plan.key, Fretboard {}, plan.difficulty, {}, cache).sections.size () == 8);
        });

        CAPTURE (root, taken);
        REQUIRE (taken <= 100.0);
    }
}

TEST_CASE ("T07-02: the search for one chord takes 5 ms or less")
{
    for (const auto& definition : bundledData ().chords.all ())
    {
        const auto chord = algo::test::chordOf (0, definition.name);

        const double taken = bestMilliseconds ([&]
        {
            REQUIRE_FALSE (searchCandidates (chord, Fretboard {}, Difficulty::Advanced).empty ());
        });

        CAPTURE (definition.name, taken);
        REQUIRE (taken <= 5.0);
    }
}

#endif

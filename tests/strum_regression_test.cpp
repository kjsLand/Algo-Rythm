// The defect test from T01-05, now on the core strum engine (T08-02).
#include "algo/Strum.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

using namespace algo;

namespace
{
    constexpr std::uint64_t kSeeds = 2000;

    // 120 BPM uses the 16-step grid and 160 BPM uses the 8-step grid.
    constexpr int kBpms[] = { 120, 160 };

    SongPlan planAt (int bpm)
    {
        SongPlan plan;
        plan.difficulty = Difficulty::Intermediate;
        plan.bpm        = bpm;
        plan.sections   = { { "Verse", "verse", 8 }, { "Chorus", "chorus", 8 } };
        return plan;
    }
}

TEST_CASE ("Slot 0 of each strum pattern is a Down stroke")
{
    for (std::uint64_t seed = 1; seed <= kSeeds; ++seed)
    {
        CAPTURE (seed);
        for (int bpm : kBpms)
        {
            CAPTURE (bpm);
            const RngFactory factory = [seed] (Stage stage, int section)
            {
                return rngFor (seed, stage, section, 0);
            };
            for (const auto& pattern : makeStrums (planAt (bpm), factory))
                REQUIRE (pattern.slots[0] == Stroke::Down);
        }
    }
}

TEST_CASE ("The rest mask never covers slot 0")
{
    for (std::uint64_t seed = 1; seed <= kSeeds; ++seed)
    {
        CAPTURE (seed);
        Rng rng (seed);
        for (int bpm : kBpms)
        {
            const int  steps   = gridSteps (Difficulty::Intermediate, bpm);
            const auto pattern = euclidStrum (steps, steps / 2, Difficulty::Intermediate, rng);
            REQUIRE (static_cast<int> (pattern.slots.size ()) == steps);
            REQUIRE (pattern.slots[0] == Stroke::Down);
        }
    }
}

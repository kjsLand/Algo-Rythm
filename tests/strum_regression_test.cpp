#include "StrumPattern.h"

#include <catch2/catch_test_macros.hpp>

#include <numeric>

namespace
{
    constexpr unsigned int NUM_SEEDS = 2000;

    // 120 BPM uses the 16-step grid and 160 BPM uses the 8-step grid.
    constexpr int BPMS[] = { 120, 160 };
}

TEST_CASE ("Slot 0 of each strum pattern is a Down stroke")
{
    for (unsigned int seed = 1; seed <= NUM_SEEDS; ++seed)
    {
        CAPTURE (seed);

        for (int bpm : BPMS)
        {
            CAPTURE (bpm);
            REQUIRE (StrumPattern::generate (bpm, seed).pattern[0] == 'D');
        }

        REQUIRE (StrumPattern::generateRandom (seed).pattern[0] == 'D');
    }
}

TEST_CASE ("The rest mask keeps its number of rests")
{
    for (unsigned int seed = 1; seed <= NUM_SEEDS; ++seed)
    {
        CAPTURE (seed);

        for (int bpm : BPMS)
        {
            CAPTURE (bpm);
            const auto r = StrumPattern::generate (bpm, seed);

            REQUIRE ((int) r.restBits.size () == r.steps);
            REQUIRE (std::accumulate (r.restBits.begin (), r.restBits.end (), 0) == r.restHits);
        }
    }
}

#include "algo/Seeds.h"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <set>

using algo::deriveSeed;
using algo::rngFor;
using algo::Stage;

TEST_CASE ("deriveSeed gives the stored values")
{
    // These values come from a separate Python program that follows the SplitMix64 definition.
    REQUIRE (deriveSeed (0, Stage::Plan, 0, 0) == 0x2130748AAAC80268ULL);
    REQUIRE (deriveSeed (42, Stage::Chords, 0, 0) == 0x3E4A12D70B0AF178ULL);
    REQUIRE (deriveSeed (42, Stage::Chords, 1, 0) == 0xDFE05ACDAF69D627ULL);
    REQUIRE (deriveSeed (42, Stage::Chords, 1, 1) == 0xD4795FC9AF8F0E1DULL);
    REQUIRE (deriveSeed (42, Stage::Render, 2, 3) == 0xC17FC85B38CAA741ULL);
    REQUIRE (deriveSeed (0xFFFFFFFFFFFFFFFFULL, Stage::Strum, 7, 11) == 0x6FC7716A03E49F9DULL);
    REQUIRE (deriveSeed (42, Stage::Voicing, -1, 0) == 0xE3278F8845F58CF7ULL);
}

TEST_CASE ("deriveSeed gives a different seed for each combination of its inputs")
{
    const std::uint64_t masters[] = { 0, 1, 2, 3, 42, 43, 1000, 0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL };
    const Stage         stages[]  = { Stage::Plan, Stage::Chords, Stage::Voicing, Stage::Strum, Stage::Hook, Stage::Render };

    std::set<std::uint64_t> seeds;
    std::size_t             combinations = 0;

    for (auto master : masters)
        for (auto stage : stages)
            for (int section = 0; section < 13; ++section)
                for (int reroll = 0; reroll < 13; ++reroll)
                {
                    seeds.insert (deriveSeed (master, stage, section, reroll));
                    ++combinations;
                }

    REQUIRE (combinations >= 10000);
    REQUIRE (seeds.size () == combinations);
}

TEST_CASE ("the section and the reroll count do not trade places")
{
    REQUIRE (deriveSeed (42, Stage::Chords, 1, 0) != deriveSeed (42, Stage::Chords, 0, 1));
}

TEST_CASE ("rngFor gives an Rng with the derived seed")
{
    auto      a = rngFor (42, Stage::Strum, 2, 1);
    algo::Rng b (deriveSeed (42, Stage::Strum, 2, 1));

    for (int i = 0; i < 100; ++i)
        REQUIRE (a.uniformInt (0, 1000000) == b.uniformInt (0, 1000000));
}

TEST_CASE ("a change to one stream does not change another stream")
{
    auto chords = rngFor (42, Stage::Chords, 1, 0);
    auto strum  = rngFor (42, Stage::Strum, 1, 0);
    auto other  = rngFor (42, Stage::Strum, 1, 0);

    for (int i = 0; i < 100; ++i)
        chords.uniformInt (0, 99);

    for (int i = 0; i < 100; ++i)
        REQUIRE (strum.uniformInt (0, 1000000) == other.uniformInt (0, 1000000));
}

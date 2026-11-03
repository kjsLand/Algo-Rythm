#include "algo/Data.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>

using namespace algo;

TEST_CASE ("The file contains 12 or more strum patterns")
{
    REQUIRE (loadBundledData ().strums.size () >= 12);
}

TEST_CASE ("Each bundled strum pattern obeys the strum rules")
{
    for (const auto& strum : loadBundledData ().strums)
    {
        CAPTURE (strum.name);
        const auto& pattern = strum.pattern;

        REQUIRE (static_cast<int> (pattern.slots.size ()) == pattern.steps);
        REQUIRE (static_cast<int> (pattern.accent.size ()) == pattern.steps);
        REQUIRE (pattern.slots[0] == Stroke::Down);

        // REQ-08-2: an even slot is Down or silent, and an odd slot is Up or silent.
        for (std::size_t i = 0; i < pattern.slots.size (); ++i)
        {
            CAPTURE (i);
            const Stroke wrong = i % 2 == 0 ? Stroke::Up : Stroke::Down;
            REQUIRE (pattern.slots[i] != wrong);
        }

        // REQ-08-6: only Advanced patterns contain Mute or Chuck strokes.
        const bool percussive = std::any_of (pattern.slots.begin (), pattern.slots.end (), [] (Stroke stroke)
        {
            return stroke == Stroke::Mute || stroke == Stroke::Chuck;
        });
        if (strum.difficulty != Difficulty::Advanced)
            REQUIRE_FALSE (percussive);
    }
}

TEST_CASE ("Each Beginner pattern has 8 steps and no rest on beat 3")
{
    for (const auto& strum : loadBundledData ().strums)
    {
        if (strum.difficulty != Difficulty::Beginner)
            continue;

        CAPTURE (strum.name);
        REQUIRE (strum.pattern.steps == 8);
        REQUIRE (strum.pattern.slots[4] != Stroke::Rest); // REQ-08-4
    }
}

TEST_CASE ("Each difficulty has patterns")
{
    const auto strums = loadBundledData ().strums;

    for (auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
    {
        REQUIRE (std::any_of (strums.begin (), strums.end (), [difficulty] (const StrumDefinition& strum)
        {
            return strum.difficulty == difficulty;
        }));
    }
}

TEST_CASE ("The accents follow the rule from T08-04")
{
    for (const auto& strum : loadBundledData ().strums)
    {
        CAPTURE (strum.name);
        const auto& pattern = strum.pattern;
        const auto  beat    = pattern.slots.size () / 4;

        for (std::size_t i = 0; i < pattern.slots.size (); ++i)
        {
            CAPTURE (i);
            if (pattern.slots[i] == Stroke::Rest)
                REQUIRE (pattern.accent[i] == 0.0);
            else if (i == beat || i == 3 * beat)
                REQUIRE (pattern.accent[i] == 1.0);
            else
                REQUIRE (pattern.accent[i] == 0.6);
        }
    }
}

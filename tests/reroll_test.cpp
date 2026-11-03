#include "algo/Engine.h"
#include "algo/Files.h"
#include "algo/Validate.h"

#include "support/PropertyTest.h"
#include "support/StarterHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

using namespace algo;

namespace
{
    void requireOnlyChanged (const SongStarter& before, const SongStarter& after, Stage stage, std::size_t index)
    {
        REQUIRE (before.key == after.key);
        REQUIRE (before.bpm == after.bpm);
        REQUIRE (before.hook == after.hook);
        REQUIRE (before.sections.size () == after.sections.size ());
        for (std::size_t i = 0; i < before.sections.size (); ++i)
        {
            const bool selected = i == index;
            if (!selected || stage != Stage::Strum)
                REQUIRE (before.sections[i].strum == after.sections[i].strum);
            if (!selected || (stage != Stage::Chords && stage != Stage::Voicing))
                REQUIRE (before.sections[i].bars == after.sections[i].bars);
            if (selected && stage == Stage::Voicing)
                for (std::size_t b = 0; b < before.sections[i].bars.size (); ++b)
                    REQUIRE (before.sections[i].bars[b].chords == after.sections[i].bars[b].chords);
        }
    }
}

TEST_CASE ("A reroll changes only the item and section that the user selected (REQ-14-2)")
{
    const std::uint64_t count = std::min<std::uint64_t> (algo::test::propertySeedCount (), 1000);
    for (std::uint64_t seed = 1; seed <= count; ++seed)
    {
        CAPTURE (seed);
        const auto starter = generate (algo::test::settingsFor (seed, static_cast<Difficulty> (seed % 3)));
        const auto index   = static_cast<std::size_t> (seed % starter.sections.size ());

        for (const auto stage : { Stage::Chords, Stage::Voicing, Stage::Strum })
        {
            CAPTURE (static_cast<int> (stage));
            const auto after = reroll (starter, stage, static_cast<int> (index));
            requireOnlyChanged (starter, after, stage, index);
            REQUIRE (rerollCount (after.rerolls, stage, static_cast<int> (index)) == 1);
            REQUIRE (validateStarter (after).empty ());
        }
    }
}

TEST_CASE ("A strum reroll uses the same stream as generate with the reroll count")
{
    auto       settings = algo::test::settingsFor (17, Difficulty::Intermediate);
    const auto starter  = generate (settings);
    const auto after    = reroll (starter, Stage::Strum, 1);

    settings.rerolls.strums = { 0, 1 };
    REQUIRE (generate (settings).sections[1].strum == after.sections[1].strum);
}

TEST_CASE ("A hook reroll changes only the hook")
{
    const auto starter = generate (algo::test::settingsFor (21, Difficulty::Beginner, true));
    const auto after   = reroll (starter, Stage::Hook, 0);
    REQUIRE (after.rerolls.hook == 1);
    REQUIRE (after.sections == starter.sections);
    REQUIRE (hookViolations (after).empty ());
}

TEST_CASE ("A saved song starter with reroll counts reloads to the same result")
{
    auto starter = generate (algo::test::settingsFor (4, Difficulty::Advanced));
    starter      = reroll (starter, Stage::Chords, 0);
    starter      = reroll (starter, Stage::Strum, 1);

    const auto loaded = parseStarter (starterJsonText (starter));
    REQUIRE (loaded == starter);
    REQUIRE (loaded.rerolls.chords == std::vector<int> { 1 });
    REQUIRE (loaded.rerolls.strums == std::vector<int> { 0, 1 });
}

TEST_CASE ("A reroll of an unknown section or of the Plan stage is an error")
{
    const auto starter = generate (GenerationSettings {});
    REQUIRE_THROWS_AS (reroll (starter, Stage::Plan, 0), std::invalid_argument);
    REQUIRE_THROWS_AS (reroll (starter, Stage::Strum, 9), std::invalid_argument);
}

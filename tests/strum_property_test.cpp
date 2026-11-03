#include "algo/Strum.h"
#include "algo/Validate.h"

#include "support/PropertyTest.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

using namespace algo;

namespace
{
    SongPlan planFor (std::uint64_t seed, Difficulty difficulty)
    {
        SongPlan plan;
        plan.difficulty = difficulty;
        plan.bpm        = 40 + static_cast<int> (seed % 90) * 2;
        plan.sections   = { { "Verse", "verse", 8 }, { "Chorus", "chorus", 8 }, { "Bridge", "bridge", 4 } };
        return plan;
    }

    RngFactory factoryFor (std::uint64_t seed, int verseRerolls)
    {
        return [seed, verseRerolls] (Stage stage, int section)
        {
            return rngFor (seed, stage, section, section == 0 ? verseRerolls : 0);
        };
    }
}

TEST_CASE ("Strum: each pattern of each seed obeys REQ-08-1 to REQ-08-6")
{
    const auto library = StrumLibrary::fromData ();
    algo::test::forEachSeed ([&library] (std::uint64_t seed)
    {
        for (const auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
        {
            CAPTURE (static_cast<int> (difficulty));
            const auto plan     = planFor (seed, difficulty);
            const auto patterns = makeStrums (plan, factoryFor (seed, 0), library);
            REQUIRE (patterns.size () == plan.sections.size ());

            for (const auto& pattern : patterns)
            {
                CAPTURE (strumText (pattern));
                REQUIRE (strumViolations (pattern, difficulty, plan.bpm).empty ());
            }
            REQUIRE (chorusDensityOk (patterns[0], patterns[1]));
        }
    });
}

TEST_CASE ("A reroll of the verse strum does not change the chorus strum or the base hit count")
{
    const auto library = StrumLibrary::fromData ();
    algo::test::forEachSeed ([&library] (std::uint64_t seed)
    {
        const auto plan     = planFor (seed, Difficulty::Intermediate);
        const auto original = makeStrums (plan, factoryFor (seed, 0), library);
        const auto rerolled = makeStrums (plan, factoryFor (seed, 1), library);

        REQUIRE (songBaseHits (plan, factoryFor (seed, 0)) == songBaseHits (plan, factoryFor (seed, 1)));
        REQUIRE (original[1] == rerolled[1]);
        REQUIRE (original[2] == rerolled[2]);
    });
}

#include "algo/Euclid.h"
#include "algo/Json.h"
#include "algo/Strum.h"
#include "algo/Validate.h"

#include "support/PropertyTest.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>

using namespace algo;

namespace
{
    SongPlan planWith (Difficulty difficulty, int bpm, const std::vector<FormSection>& sections = { { "Verse", "verse", 8 }, { "Chorus", "chorus", 8 } })
    {
        SongPlan plan;
        plan.difficulty = difficulty;
        plan.bpm        = bpm;
        plan.sections   = sections;
        return plan;
    }

    RngFactory factoryFor (std::uint64_t seed, std::vector<int> strumRerolls = {})
    {
        return [seed, strumRerolls] (Stage stage, int section)
        {
            const int count = section >= 0 && section < static_cast<int> (strumRerolls.size ()) ? strumRerolls[static_cast<std::size_t> (section)] : 0;
            return rngFor (seed, stage, section, count);
        };
    }

    int countOf (const StrumPattern& pattern, Stroke stroke)
    {
        return static_cast<int> (std::count (pattern.slots.begin (), pattern.slots.end (), stroke));
    }
}

TEST_CASE ("The grid has 8 steps at Beginner and above 140 BPM, and 16 steps at other levels (REQ-08-3)")
{
    REQUIRE (gridSteps (Difficulty::Beginner, 90) == 8);
    REQUIRE (gridSteps (Difficulty::Beginner, 200) == 8);
    REQUIRE (gridSteps (Difficulty::Intermediate, 140) == 16);
    REQUIRE (gridSteps (Difficulty::Intermediate, 142) == 8);
    REQUIRE (gridSteps (Difficulty::Advanced, 100) == 16);
    REQUIRE (gridSteps (Difficulty::Advanced, 160) == 8);
    REQUIRE (gridSteps (Difficulty::Beginner, 90, TimeSig { 6, 8 }) == 12);
}

TEST_CASE ("A Euclidean strum has slot 0 on Down and beat 3 sounds at Beginner (REQ-08-1, REQ-08-4)")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        Rng rng (seed);
        for (const auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
        {
            const int  steps   = difficulty == Difficulty::Beginner ? 8 : 16;
            const int  target  = rng.uniformInt (1, steps);
            const auto pattern = euclidStrum (steps, target, difficulty, rng);

            REQUIRE (pattern.slots[0] == Stroke::Down);
            REQUIRE (hitCount (pattern) == std::max (target, difficulty == Difficulty::Beginner ? 2 : 1));
            if (difficulty == Difficulty::Beginner)
                REQUIRE (pattern.slots[static_cast<std::size_t> (steps / 2)] != Stroke::Rest);

            for (int i = 0; i < steps; ++i)
            {
                const auto stroke = pattern.slots[static_cast<std::size_t> (i)];
                REQUIRE ((stroke == Stroke::Rest || stroke == (i % 2 == 0 ? Stroke::Down : Stroke::Up)));
            }
        }
    });
}

TEST_CASE ("The defect from T01-05: no generated pattern has a silent beat 1")
{
    for (std::uint64_t seed = 1; seed <= 2000; ++seed)
    {
        for (const auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
            for (const int bpm : { 80, 120, 160 })
            {
                const auto patterns = makeStrums (planWith (difficulty, bpm), factoryFor (seed));
                for (const auto& pattern : patterns)
                    REQUIRE (pattern.slots[0] == Stroke::Down);
            }
    }
}

TEST_CASE ("The text and the ruler of a pattern")
{
    StrumPattern pattern;
    pattern.steps = 8;
    pattern.slots = { Stroke::Down, Stroke::Rest, Stroke::Down, Stroke::Up, Stroke::Rest, Stroke::Up, Stroke::Down, Stroke::Up };
    REQUIRE (strumText (pattern) == "D - D U - U D U");
    REQUIRE (strumRuler (pattern) == "1 + 2 + 3 + 4 +");

    pattern.steps = 16;
    pattern.slots.assign (16, Stroke::Rest);
    REQUIRE (strumRuler (pattern) == "1 e + a 2 e + a 3 e + a 4 e + a");
}

TEST_CASE ("The library filter uses the difficulty, the step count and the hit count")
{
    const auto library = StrumLibrary::fromData ();

    for (const auto& def : library.filter (Difficulty::Beginner, 8, 5))
    {
        REQUIRE (def.difficulty == Difficulty::Beginner);
        REQUIRE (def.pattern.steps == 8);
        REQUIRE (hitCount (def.pattern) == 5);
    }
    REQUIRE_FALSE (library.filter (Difficulty::Beginner, 8, 5).empty ());
    REQUIRE (library.filter (Difficulty::Beginner, 16, 8).empty ());

    Rng rng (1);
    REQUIRE_FALSE (library.pickLibraryPattern (Difficulty::Beginner, 8, 3, rng).has_value ());
    REQUIRE (library.pickLibraryPattern (Difficulty::Beginner, 8, 4, rng).has_value ());
}

TEST_CASE ("A learned weight changes the library choice and a weight must be positive")
{
    auto library = StrumLibrary::fromData ();
    REQUIRE_THROWS (library.setWeights ({ { "Folk Push", 0.0 } }));

    library.setWeights ({ { "Folk Push", 1000.0 } });
    int folkPush = 0;
    for (std::uint64_t seed = 1; seed <= 200; ++seed)
    {
        Rng rng (seed);
        if (library.pickLibraryPattern (Difficulty::Beginner, 8, 5, rng)->name == "Folk Push")
            ++folkPush;
    }
    REQUIRE (folkPush > 190);
}

TEST_CASE ("30% or more of the Beginner patterns come from the library")
{
    const auto          library = StrumLibrary::fromData ();
    const std::uint64_t seeds   = 10000;
    int                 fromLib = 0;
    int                 total   = 0;
    for (std::uint64_t seed = 1; seed <= seeds; ++seed)
    {
        for (const auto& pattern : makeStrums (planWith (Difficulty::Beginner, 100), factoryFor (seed), library))
        {
            ++total;
            if (!pattern.name.empty ())
                ++fromLib;
        }
    }
    INFO (fromLib << " of " << total);
    REQUIRE (fromLib * 10 >= total * 3);
}

TEST_CASE ("A chorus has 1 or 2 more hits than the verse (REQ-08-5)")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        for (const auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
        {
            const auto patterns = makeStrums (planWith (difficulty, 90 + static_cast<int> (seed % 100)), factoryFor (seed));
            REQUIRE (chorusDensityOk (patterns[0], patterns[1]));
            const int extra = hitCount (patterns[1]) - hitCount (patterns[0]);
            REQUIRE ((extra == 1 || extra == 2));
        }
    });
}

TEST_CASE ("Each accent is in the range 0 to 1, and beats 2 and 4 have the accent 1.0")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        for (const auto& pattern : makeStrums (planWith (Difficulty::Intermediate, 100), factoryFor (seed)))
        {
            REQUIRE (static_cast<int> (pattern.accent.size ()) == pattern.steps);
            for (int i = 0; i < pattern.steps; ++i)
            {
                const auto a      = pattern.accent[static_cast<std::size_t> (i)];
                const auto stroke = pattern.slots[static_cast<std::size_t> (i)];
                REQUIRE (a >= 0.0);
                REQUIRE (a <= 1.0);
                if (stroke == Stroke::Rest)
                    REQUIRE (a == 0.0);
                else if (i == pattern.steps / 4 || i == 3 * pattern.steps / 4)
                    REQUIRE (a == kBackbeatAccent);
                else
                    REQUIRE (a == kHitAccent);
            }
        }
    });
}

TEST_CASE ("Only Advanced patterns contain Mute or Chuck strokes (REQ-08-6)")
{
    int chucks = 0;
    int mutes  = 0;
    algo::test::forEachSeed ([&] (std::uint64_t seed)
    {
        for (const auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
        {
            for (const auto& pattern : makeStrums (planWith (difficulty, 100), factoryFor (seed)))
            {
                const int percussion = countOf (pattern, Stroke::Mute) + countOf (pattern, Stroke::Chuck);
                if (difficulty != Difficulty::Advanced)
                    REQUIRE (percussion == 0);
                if (difficulty == Difficulty::Advanced)
                {
                    chucks += countOf (pattern, Stroke::Chuck) > 0 ? 1 : 0;
                    mutes += countOf (pattern, Stroke::Mute) > 0 ? 1 : 0;
                }
                REQUIRE (pattern.slots[0] == Stroke::Down);
            }
        }
    });
    REQUIRE (chucks >= 1);
    REQUIRE (mutes >= 1);
}

TEST_CASE ("A Beginner strum uses 6 strings and other levels use 4 or 6 strings")
{
    bool four = false;
    for (std::uint64_t seed = 1; seed <= 200; ++seed)
    {
        for (const auto& pattern : makeStrums (planWith (Difficulty::Beginner, 100), factoryFor (seed)))
            REQUIRE (pattern.strings == 6);
        for (const auto& pattern : makeStrums (planWith (Difficulty::Advanced, 100), factoryFor (seed)))
        {
            REQUIRE ((pattern.strings == 4 || pattern.strings == 6));
            four = four || pattern.strings == 4;
        }
    }
    REQUIRE (four);
}

TEST_CASE ("A rest mask that cannot rotate loses rests until slot 0 sounds")
{
    // With 2 steps and 1 rest at Beginner, slots 0 and 1 are both protected.
    for (std::uint64_t seed = 1; seed <= 100; ++seed)
    {
        Rng        rng (seed);
        const auto pattern = euclidStrum (2, 1, Difficulty::Beginner, rng);
        REQUIRE (pattern.slots[0] == Stroke::Down);
        REQUIRE (pattern.slots[1] == Stroke::Up);
    }
    Rng rng (1);
    REQUIRE_THROWS (euclidStrum (1, 1, Difficulty::Beginner, rng));
}

TEST_CASE ("A 6/8 pattern has 12 steps and the strong beats on slots 0 and 6")
{
    auto plan    = planWith (Difficulty::Beginner, 90);
    plan.timeSig = { 6, 8 };
    for (std::uint64_t seed = 1; seed <= 500; ++seed)
    {
        for (const auto& pattern : makeStrums (plan, factoryFor (seed)))
        {
            REQUIRE (pattern.steps == 12);
            REQUIRE (pattern.slots[0] == Stroke::Down);
            REQUIRE (pattern.slots[6] != Stroke::Rest);
            REQUIRE (pattern.accent[6] == kBackbeatAccent);
            REQUIRE (strumViolations (pattern, Difficulty::Beginner, 90, plan.timeSig).empty ());
        }
    }
    StrumPattern pattern;
    pattern.steps = 12;
    pattern.slots.assign (12, Stroke::Rest);
    REQUIRE (strumRuler (pattern, TimeSig { 6, 8 }) == "1 + 2 + 3 + 4 + 5 + 6 +");
}

TEST_CASE ("Fingerpicking patterns use Pluck strokes and the library strum filter ignores them (T16-06)")
{
    const auto library = StrumLibrary::fromData ();
    for (const auto& def : library.all ())
    {
        if (def.pattern.pluck.empty ())
            continue;
        CAPTURE (def.name);
        REQUIRE (def.pattern.slots[0] == Stroke::Pluck);
        REQUIRE (def.pattern.pluck.size () == def.pattern.slots.size ());
        for (int hits = 0; hits <= def.pattern.steps; ++hits)
            for (const auto& other : library.filter (Difficulty::Advanced, def.pattern.steps, hits))
                REQUIRE (other.name != def.name);
    }

    auto plan          = planWith (Difficulty::Intermediate, 100);
    plan.fingerpicking = true;
    for (std::uint64_t seed = 1; seed <= 200; ++seed)
        for (const auto& pattern : makeStrums (plan, factoryFor (seed)))
        {
            REQUIRE (pattern.steps == 8);
            REQUIRE_FALSE (pattern.pluck.empty ());
            REQUIRE (strumViolations (pattern, Difficulty::Intermediate, 100).empty ());
        }
}

TEST_CASE ("A Pluck finds its string on the voicing")
{
    Voicing g; // 320003
    g.frets = { 3, 2, 0, 0, 0, 3 };
    REQUIRE (pluckStringIndex (g, kPluckBass) == 0);
    REQUIRE (pluckStringIndex (g, kPluckAltBass) == 2);
    REQUIRE (pluckStringIndex (g, 1) == 5);
    REQUIRE (pluckStringIndex (g, 3) == 3);

    Voicing c; // x32010
    c.frets = { kMutedString, 3, 2, 0, 1, 0 };
    REQUIRE (pluckStringIndex (c, kPluckBass) == 1);
    REQUIRE (pluckStringIndex (c, kPluckAltBass) == 2);
    REQUIRE (pluckStringIndex (c, 6) == 1); // The muted low E string moves up

    Voicing none;
    REQUIRE (pluckStringIndex (none, kPluckBass) == -1);
}

TEST_CASE ("Pluck slots write and read as their string characters")
{
    StrumPattern pattern;
    REQUIRE (setSlots (pattern, "b3a2-1x") == 0);
    REQUIRE (pattern.slots[0] == Stroke::Pluck);
    REQUIRE (pluckAt (pattern, 0) == kPluckBass);
    REQUIRE (pluckAt (pattern, 2) == kPluckAltBass);
    REQUIRE (pluckAt (pattern, 3) == 2);
    REQUIRE (pattern.slots[4] == Stroke::Rest);
    REQUIRE (pattern.slots[6] == Stroke::Mute);
    REQUIRE (strumText (pattern) == "b 3 a 2 - 1 x");
    REQUIRE (setSlots (pattern, "D?") == '?');

    StrumPattern plain;
    REQUIRE (setSlots (plain, "D-DU") == 0);
    REQUIRE (plain.pluck.empty ());
}

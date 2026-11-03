#include "algo/Engine.h"
#include "algo/Strum.h"
#include "algo/Validate.h"
#include "algo/VoicingCache.h"

#include "support/PropertyTest.h"
#include "support/StarterHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <chrono>
#include <limits>

using namespace algo;
using Catch::Matchers::ContainsSubstring;

TEST_CASE ("generate with default settings returns a song starter with 2 or more sections")
{
    const auto starter = generate (GenerationSettings {});
    REQUIRE (starter.sections.size () >= 2);
    REQUIRE (starter.hook.empty ());
    for (const auto& section : starter.sections)
    {
        REQUIRE_FALSE (section.bars.empty ());
        REQUIRE_FALSE (section.role.empty ());
        for (const auto& bar : section.bars)
            REQUIRE (bar.voicings.size () == bar.chords.size ());
    }
}

TEST_CASE ("Settings that are not valid throw GenerationError with all error texts")
{
    GenerationSettings settings;
    settings.bpmMin = 10;
    settings.form   = "Nope";
    try
    {
        generate (settings);
        FAIL ("generate did not throw");
    }
    catch (const GenerationError& error)
    {
        REQUIRE_THAT (error.what (), ContainsSubstring ("BPM"));
        REQUIRE_THAT (error.what (), ContainsSubstring ("Unknown form: Nope"));
    }
}

TEST_CASE ("The same settings give the same song starter")
{
    const auto settings = algo::test::settingsFor (31, Difficulty::Advanced, true);
    REQUIRE (generate (settings) == generate (settings));

    auto other       = settings;
    other.masterSeed = 32;
    REQUIRE_FALSE (generate (settings) == generate (other));
}

TEST_CASE ("The song starter copies the settings")
{
    auto settings           = algo::test::settingsFor (8, Difficulty::Intermediate);
    settings.capo           = 3;
    settings.rerolls.strums = { 0, 2 };
    const auto starter      = generate (settings);
    REQUIRE (starter.seed == 8);
    REQUIRE (starter.difficulty == Difficulty::Intermediate);
    REQUIRE (starter.capo == 3);
    REQUIRE (starter.form == settings.form);
    REQUIRE (starter.rerolls == settings.rerolls);
    REQUIRE (starter.bpm >= settings.bpmMin);
    REQUIRE (starter.bpm <= settings.bpmMax);
}

TEST_CASE ("The stats have a time for each stage")
{
    GenerationStats stats;
    generate (GenerationSettings {}, bundledData (), &stats);
    REQUIRE (stats.totalMs > 0.0);
    REQUIRE (stats.totalMs >= stats.voicingMs);
}

TEST_CASE ("A generated song starter passes validateStarter for each seed")
{
    VoicingCache cache;
    algo::test::forEachSeed ([&cache] (std::uint64_t seed)
    {
        for (const auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
        {
            CAPTURE (static_cast<int> (difficulty));
            const auto starter  = generate (algo::test::settingsFor (seed, difficulty, seed % 3 == 0), bundledData (), nullptr, &cache);
            const auto failures = validateStarter (starter);
            for (const auto& failure : failures)
                UNSCOPED_INFO (failureText (failure));
            REQUIRE (failures.empty ());
        }
    });
}

TEST_CASE ("A tuning that is not Standard gives voicings that pass the filter (T16-07)")
{
    for (const std::string tuning : { "Drop D", "DADGAD", "Open G", "Open D", "Half-step down" })
    {
        CAPTURE (tuning);
        for (std::uint64_t seed = 1; seed <= 30; ++seed)
        {
            auto settings      = algo::test::settingsFor (seed, static_cast<Difficulty> (seed % 3));
            settings.tuning    = tuning;
            const auto starter = generate (settings);
            REQUIRE (starter.tuning.name == tuning);

            const auto failures = validateStarter (starter);
            for (const auto& failure : failures)
                UNSCOPED_INFO (failureText (failure));
            REQUIRE (failures.empty ());
        }
    }

    GenerationSettings settings;
    settings.tuning = "Nope";
    REQUIRE_THROWS_AS (generate (settings), GenerationError);
}

TEST_CASE ("A 6/8 song starter has 12-step strums and 6/8 bars (T16-06)")
{
    for (std::uint64_t seed = 1; seed <= 50; ++seed)
    {
        auto settings      = algo::test::settingsFor (seed, static_cast<Difficulty> (seed % 3), true);
        settings.timeSig   = { 6, 8 };
        const auto starter = generate (settings);
        REQUIRE (starter.timeSig == TimeSig { 6, 8 });
        for (const auto& section : starter.sections)
            REQUIRE (section.strum.steps == 12);
        REQUIRE (validateStarter (starter).empty ());
    }
}

// A speed limit has a meaning only in an optimized build.
#ifdef NDEBUG
TEST_CASE ("A 64-bar Advanced song starter takes 200 ms or less")
{
    DataSet data = bundledData ();
    data.forms.push_back ({ "Long", { { "Verse", "verse", 16 }, { "Chorus", "chorus", 16 }, { "Verse 2", "verse", 16 }, { "Bridge", "bridge", 16 } } });

    GenerationSettings settings;
    settings.form       = "Long";
    settings.difficulty = Difficulty::Advanced;

    double best = std::numeric_limits<double>::max ();
    for (std::uint64_t seed = 1; seed <= 5; ++seed)
    {
        settings.masterSeed                                     = seed;
        const auto                                      start   = std::chrono::steady_clock::now ();
        const auto                                      starter = generate (settings, data);
        const std::chrono::duration<double, std::milli> taken   = std::chrono::steady_clock::now () - start;
        best                                                    = std::min (best, taken.count ());

        int bars = 0;
        for (const auto& section : starter.sections)
            bars += static_cast<int> (section.bars.size ());
        REQUIRE (bars == 64);
    }
    REQUIRE (best <= 200.0);
}
#endif

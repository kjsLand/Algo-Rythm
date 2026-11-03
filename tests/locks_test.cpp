#include "algo/Engine.h"
#include "algo/VoicingCache.h"

#include "support/PropertyTest.h"
#include "support/StarterHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>

using namespace algo;

namespace
{
    std::uint64_t lockSeedCount ()
    {
        return std::min<std::uint64_t> (algo::test::propertySeedCount (), 1000);
    }

    // The previous song starter, then the settings of the next click on Generate.
    struct Pair
    {
        SongStarter        previous;
        GenerationSettings next;
    };

    Pair pairFor (std::uint64_t seed, VoicingCache& cache)
    {
        auto       settings = algo::test::settingsFor (seed, static_cast<Difficulty> (seed % 3));
        const auto previous = generate (settings, bundledData (), nullptr, &cache);
        settings.masterSeed = seed + 100000;
        settings.previous   = std::make_shared<SongStarter> (previous);
        return { previous, settings };
    }
}

TEST_CASE ("A locked key, scale, tempo or form does not change on Generate (REQ-14-1)")
{
    VoicingCache cache;
    for (std::uint64_t seed = 1; seed <= lockSeedCount (); ++seed)
    {
        CAPTURE (seed);
        auto pair = pairFor (seed, cache);

        auto key           = pair.next;
        key.locks.key      = true;
        key.locks.scale    = true;
        const auto withKey = generate (key, bundledData (), nullptr, &cache);
        REQUIRE (withKey.key == pair.previous.key);

        auto tempo        = pair.next;
        tempo.locks.tempo = true;
        REQUIRE (generate (tempo, bundledData (), nullptr, &cache).bpm == pair.previous.bpm);

        auto form           = pair.next;
        form.form           = "Verse-Chorus-Bridge";
        form.locks.form     = true;
        const auto withForm = generate (form, bundledData (), nullptr, &cache);
        REQUIRE (withForm.form == pair.previous.form);
        REQUIRE (withForm.sections.size () == pair.previous.sections.size ());
    }
}

TEST_CASE ("A locked section keeps its chords, voicings and strum (REQ-14-1)")
{
    VoicingCache cache;
    for (std::uint64_t seed = 1; seed <= lockSeedCount (); ++seed)
    {
        CAPTURE (seed);
        auto pair                = pairFor (seed, cache);
        pair.next.locks.key      = true;
        pair.next.locks.scale    = true;
        pair.next.locks.tempo    = true;
        pair.next.locks.form     = true;
        pair.next.locks.sections = { { true, true, true } };

        const auto next = generate (pair.next, bundledData (), nullptr, &cache);
        REQUIRE (next.sections[0].bars == pair.previous.sections[0].bars);
        REQUIRE (next.sections[0].strum == pair.previous.sections[0].strum);

        // Only the strum lock
        pair.next.locks.sections = { { false, false, true } };
        REQUIRE (generate (pair.next, bundledData (), nullptr, &cache).sections[0].strum == pair.previous.sections[0].strum);

        // Only the chord lock: the chords stay and the voicings follow them.
        pair.next.locks.sections = { { true, false, false } };
        const auto chords        = generate (pair.next, bundledData (), nullptr, &cache);
        for (std::size_t b = 0; b < chords.sections[0].bars.size (); ++b)
            REQUIRE (chords.sections[0].bars[b].chords == pair.previous.sections[0].bars[b].chords);
    }
}

TEST_CASE ("A locked verse progression stays the same after 10 clicks on Generate")
{
    GenerationSettings settings;
    settings.masterSeed = 1;
    auto       previous = generate (settings);
    const auto verse    = previous.sections[0].bars;

    settings.locks.key      = true;
    settings.locks.scale    = true;
    settings.locks.sections = { { true, false, false } };
    for (std::uint64_t click = 2; click <= 11; ++click)
    {
        settings.masterSeed = click;
        settings.previous   = std::make_shared<SongStarter> (previous);
        previous            = generate (settings);
        for (std::size_t b = 0; b < verse.size (); ++b)
            REQUIRE (previous.sections[0].bars[b].chords == verse[b].chords);
    }
}

TEST_CASE ("A lock with no previous song starter has no effect")
{
    GenerationSettings settings;
    settings.masterSeed = 9;
    const auto plain    = generate (settings);

    settings.locks.key      = true;
    settings.locks.tempo    = true;
    settings.locks.sections = { { true, true, true } };
    REQUIRE (generate (settings) == plain);
}

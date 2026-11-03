#include "algo/Engine.h"
#include "algo/Hook.h"
#include "algo/Render.h"
#include "algo/Smf.h"
#include "algo/Validate.h"
#include "algo/VoicingCache.h"

#include "support/PropertyTest.h"
#include "support/SmfReader.h"
#include "support/StarterHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <map>

using namespace algo;

TEST_CASE ("The hook is off by default (REQ-13-6)")
{
    REQUIRE_FALSE (GenerationSettings {}.hook);
    REQUIRE (generate (GenerationSettings {}).hook.empty ());
}

TEST_CASE ("Each hook bar has 5 to 9 note starts on a 16-step grid")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        Rng        rng (seed);
        const int  bars   = rng.uniformInt (2, 4);
        const auto starts = hookRhythm (bars, TimeSig {}, rng);

        std::map<Tick, int> perBar;
        for (auto start : starts)
        {
            REQUIRE (start % kHookStepTicks == 0);
            ++perBar[start / 1920];
        }
        REQUIRE (static_cast<int> (perBar.size ()) == bars);
        for (const auto& [bar, count] : perBar)
        {
            REQUIRE (count >= kMinHookHits);
            REQUIRE (count <= kMaxHookHits);
        }
    });
}

TEST_CASE ("Hook: each hook obeys REQ-13-1 to REQ-13-5")
{
    VoicingCache cache;
    algo::test::forEachSeed ([&cache] (std::uint64_t seed)
    {
        const auto difficulty = static_cast<Difficulty> (seed % 3);
        const auto starter    = generate (algo::test::settingsFor (seed, difficulty, true), bundledData (), nullptr, &cache);
        REQUIRE (starter.hook.size () >= 10);

        const auto failures = hookViolations (starter);
        for (const auto& failure : failures)
            UNSCOPED_INFO (failureText (failure));
        REQUIRE (failures.empty ());

        // The hook is in the chorus, or in the first section if no chorus exists.
        std::vector<std::string> roles;
        for (const auto& section : starter.sections)
            roles.push_back (section.role);
        const auto starts = sectionStarts (starter);
        const auto index  = static_cast<std::size_t> (hookSectionIndex (roles));
        REQUIRE (starter.hook.front ().start >= starts[index]);
        REQUIRE (starter.hook.front ().start < starts[index] + 1920);

        const auto [low, high] = std::minmax_element (starter.hook.begin (), starter.hook.end (), [] (const MelodyNote& a, const MelodyNote& b)
        {
            return a.pitch < b.pitch;
        });
        REQUIRE (high->pitch - low->pitch <= kHookRange);
        REQUIRE (starter.hook.back ().length == kLastHookNote);
        for (const auto& note : starter.hook)
            REQUIRE ((note.velocity == kStrongHookVelocity || note.velocity == kWeakHookVelocity));
    });
}

TEST_CASE ("With the hook on, the MIDI file has a hook track with 10 or more notes")
{
    const auto starter = generate (algo::test::settingsFor (3, Difficulty::Beginner, true));
    const auto file    = algo::test::readSmf (smfBytes (render (starter)));

    int hookNotes = 0;
    for (const auto& event : file.tracks[kHookTrack].events)
        if ((event.status & 0xF0) == 0x90)
            ++hookNotes;
    REQUIRE (hookNotes >= 10);
    REQUIRE (file.tracks[kHookTrack].events[1].status == (0xC0 | kHookChannel));
    REQUIRE (file.tracks[kHookTrack].events[1].data[0] == kHookProgram);
}

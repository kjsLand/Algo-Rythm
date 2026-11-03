#include "algo/Engine.h"
#include "algo/Render.h"
#include "algo/Smf.h"
#include "algo/VoicingCache.h"

#include "support/PropertyTest.h"
#include "support/SmfReader.h"
#include "support/StarterHelpers.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>

using namespace algo;

TEST_CASE ("Round trip: render, write, read, then the note count and the tempo are the same")
{
    VoicingCache        cache;
    const std::uint64_t count = std::min<std::uint64_t> (algo::test::propertySeedCount (), 1000);
    for (std::uint64_t seed = 1; seed <= count; ++seed)
    {
        CAPTURE (seed);
        const auto difficulty = static_cast<Difficulty> (seed % 3);
        const auto starter    = generate (algo::test::settingsFor (seed, difficulty, seed % 2 == 0), bundledData (), nullptr, &cache);
        const auto events     = render (starter);
        const auto file       = algo::test::readSmf (smfBytes (events));

        REQUIRE (file.noteCount () == static_cast<int> (events.notes.size ()));
        REQUIRE (file.bpm () == Catch::Approx (starter.bpm).margin (0.01));
        REQUIRE (file.tracks.size () == static_cast<std::size_t> (kTrackCount));
        REQUIRE (file.metaTexts (0x06).size () == starter.sections.size ());
    }
}

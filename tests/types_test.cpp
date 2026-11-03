#include "algo/Types.h"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace algo;

TEST_CASE ("A default SongStarter has schemaVersion 1")
{
    const SongStarter starter;

    REQUIRE (starter.schemaVersion == 1);
    REQUIRE (kPpq == 480);
}

TEMPLATE_TEST_CASE ("Two default objects are equal", "", Key, TimeSig, ChordSymbol, FretPos, Voicing, StrumPattern, MelodyNote, Bar, Section, Tuning, SongStarter)
{
    REQUIRE (TestType {} == TestType {});
}

TEST_CASE ("operator== finds a change in a nested field")
{
    SongStarter a;
    SongStarter b;
    a.sections.push_back ({});
    b.sections.push_back ({});
    REQUIRE (a == b);

    b.sections[0].strum.slots[3] = Stroke::Up;
    REQUIRE_FALSE (a == b);
}

TEST_CASE ("A default Tuning is standard tuning from low E to high E")
{
    const Tuning tuning;

    REQUIRE (tuning.strings == std::array<MidiNote, kNumStrings> { 40, 45, 50, 55, 59, 64 });
}

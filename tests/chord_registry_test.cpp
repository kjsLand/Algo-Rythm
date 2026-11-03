#include "algo/Data.h"
#include "algo/DataError.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <stdexcept>
#include <vector>

using namespace algo;

TEST_CASE ("The registry contains 21 chord types in file order")
{
    const auto names = loadBundledData ().chords.allNames ();

    REQUIRE (names.size () == 21);
    REQUIRE (names.front () == "Major");
    REQUIRE (names[11] == "Add9");
    REQUIRE (names.back () == "Maj7#5");
}

TEST_CASE ("The chord types for the harmonic and melodic minor scales are present")
{
    const auto chords = loadBundledData ().chords;

    REQUIRE (chords.get ("mMaj7").intervals == std::vector<int> { 3, 7, 11 });
    REQUIRE (chords.get ("Maj7#5").intervals == std::vector<int> { 4, 8, 11 });
}

TEST_CASE ("The intervals of the 12 chord types from the old registry did not change")
{
    const auto chords = loadBundledData ().chords;

    REQUIRE (chords.get ("Major").intervals == std::vector<int> { 4, 7 });
    REQUIRE (chords.get ("Minor").intervals == std::vector<int> { 3, 7 });
    REQUIRE (chords.get ("Aug").intervals == std::vector<int> { 4, 8 });
    REQUIRE (chords.get ("Dim").intervals == std::vector<int> { 3, 6 });
    REQUIRE (chords.get ("Maj7").intervals == std::vector<int> { 4, 7, 11 });
    REQUIRE (chords.get ("M7").intervals == std::vector<int> { 3, 7, 10 });
    REQUIRE (chords.get ("Dom7").intervals == std::vector<int> { 4, 7, 10 });
    REQUIRE (chords.get ("Dim7").intervals == std::vector<int> { 3, 6, 9 });
    REQUIRE (chords.get ("M7b5").intervals == std::vector<int> { 3, 6, 10 });
    REQUIRE (chords.get ("Sus2").intervals == std::vector<int> { 2, 7 });
    REQUIRE (chords.get ("Sus4").intervals == std::vector<int> { 5, 7 });
    REQUIRE (chords.get ("Add9").intervals == std::vector<int> { 4, 7, 14 });
}

TEST_CASE ("Dom7 on root 7 gives the pitch classes of G7")
{
    const auto chords = loadBundledData ().chords;

    REQUIRE (ChordRegistry::pitchClasses (7, chords.get ("Dom7")) == std::vector<PitchClass> { 7, 11, 2, 5 });
}

TEST_CASE ("A voicing cannot omit the root, the 3rd or the 7th")
{
    for (const auto& chord : loadBundledData ().chords.all ())
    {
        CAPTURE (chord.name);
        for (int interval : { 3, 4, 10, 11 })
            REQUIRE (std::find (chord.optional.begin (), chord.optional.end (), interval) == chord.optional.end ());
    }
}

TEST_CASE ("Chord lookup by name is not case-sensitive and lookup by symbol is")
{
    const auto chords = loadBundledData ().chords;

    REQUIRE (chords.get ("dom7").name == "Dom7");
    REQUIRE_THROWS_AS (chords.get ("Klingon"), std::out_of_range);

    REQUIRE (chords.containsSymbol ("m7"));
    REQUIRE (chords.containsSymbol (""));
    REQUIRE_FALSE (chords.containsSymbol ("M7"));
}

TEST_CASE ("A chord with a duplicate name causes a load error")
{
    const std::string text = R"({ "schemaVersion": 1, "chords": [
        { "name": "Major", "symbol": "", "intervals": [4, 7], "optional": [], "tags": [] },
        { "name": "MAJOR", "symbol": "M", "intervals": [4, 7], "optional": [], "tags": [] } ] })";

    try
    {
        ChordRegistry::fromJsonText (text);
        FAIL ("No error");
    }
    catch (const DataError& e)
    {
        REQUIRE (e.file == "chords.json");
        REQUIRE (e.entry == "MAJOR");
        REQUIRE (e.message == "the name is not unique in the file");
    }
}

TEST_CASE ("An optional interval must be one of the chord intervals")
{
    const std::string text = R"({ "schemaVersion": 1, "chords": [
        { "name": "Major", "symbol": "", "intervals": [4, 7], "optional": [9], "tags": [] } ] })";

    REQUIRE_THROWS_AS (ChordRegistry::fromJsonText (text), DataError);
}

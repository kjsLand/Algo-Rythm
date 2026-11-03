#include "algo/Data.h"
#include "algo/DataError.h"

#include <catch2/catch_test_macros.hpp>

#include <numeric>
#include <stdexcept>
#include <vector>

using namespace algo;

TEST_CASE ("allNames returns the 15 scales in file order")
{
    const std::vector<std::string> expected {
        "Major", "Natural Minor", "Harmonic Minor", "Melodic Minor", "Dorian",
        "Phrygian", "Lydian", "Mixolydian", "Locrian", "Major Pentatonic",
        "Minor Pentatonic", "Blues", "Whole Tone", "Diminished (HW)", "Diminished (WH)"
    };

    REQUIRE (loadBundledData ().scales.allNames () == expected);
}

TEST_CASE ("The steps of each bundled scale add up to 12 and each scale has tags")
{
    for (const auto& scale : loadBundledData ().scales.all ())
    {
        CAPTURE (scale.name);
        REQUIRE (std::accumulate (scale.steps.begin (), scale.steps.end (), 0) == 12);
        REQUIRE_FALSE (scale.tags.empty ());
    }
}

TEST_CASE ("A scale with steps that do not add up to 12 causes a load error")
{
    const std::string text = R"({ "schemaVersion": 1, "scales": [
        { "name": "Good", "steps": [12], "tags": [] },
        { "name": "Bad", "steps": [2, 2, 2], "tags": [] } ] })";

    try
    {
        ScaleRegistry::fromJsonText (text);
        FAIL ("No error");
    }
    catch (const DataError& e)
    {
        REQUIRE (e.file == "scales.json");
        REQUIRE (e.entry == "Bad");
        REQUIRE (e.message == "the steps add up to 6, not 12");
    }
}

TEST_CASE ("Scale lookup is not case-sensitive")
{
    const auto scales = loadBundledData ().scales;

    REQUIRE (scales.get ("natural minor").name == "Natural Minor");
    REQUIRE (scales.get ("DORIAN").name == "Dorian");
    REQUIRE (scales.contains ("major pentatonic"));
    REQUIRE_FALSE (scales.contains ("Klingon"));
    REQUIRE_THROWS_AS (scales.get ("Klingon"), std::out_of_range);
}

TEST_CASE ("A scale gives its pitch classes in degree order")
{
    const auto scales = loadBundledData ().scales;

    REQUIRE (ScaleRegistry::pitchClasses (0, scales.get ("Major")) == std::vector<PitchClass> { 0, 2, 4, 5, 7, 9, 11 });
    REQUIRE (ScaleRegistry::pitchClasses (9, scales.get ("Minor Pentatonic")) == std::vector<PitchClass> { 9, 0, 2, 4, 7 });
}

#include "algo/DataError.h"
#include "algo/Diatonic.h"

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>
#include <vector>

using namespace algo;

namespace
{
    using Names = std::vector<std::string>;

    Names triadNames (const Key& key)
    {
        Names names;
        for (int degree = 1; degree <= 7; ++degree)
            names.push_back (diatonicTriad (key, degree).chord.name);
        return names;
    }

    Names seventhNames (const Key& key)
    {
        Names names;
        for (int degree = 1; degree <= 7; ++degree)
            names.push_back (diatonicSeventh (key, degree).chord.name);
        return names;
    }
}

TEST_CASE ("Major gives the correct triads and 7th chords")
{
    const Key key { 0, "Major" };

    REQUIRE (triadNames (key) == Names { "Major", "Minor", "Minor", "Major", "Major", "Minor", "Dim" });
    REQUIRE (seventhNames (key) == Names { "Maj7", "M7", "M7", "Maj7", "Dom7", "M7", "M7b5" });
}

TEST_CASE ("Natural Minor gives the correct triads and 7th chords")
{
    const Key key { 9, "Natural Minor" };

    REQUIRE (triadNames (key) == Names { "Minor", "Dim", "Major", "Minor", "Minor", "Major", "Major" });
    REQUIRE (seventhNames (key) == Names { "M7", "M7b5", "Maj7", "M7", "M7", "Maj7", "Dom7" });
}

TEST_CASE ("Harmonic Minor uses the mMaj7, Maj7#5 and Dim7 chord types")
{
    const Key key { 9, "Harmonic Minor" };

    REQUIRE (triadNames (key) == Names { "Minor", "Dim", "Aug", "Minor", "Major", "Major", "Dim" });
    REQUIRE (seventhNames (key) == Names { "mMaj7", "M7b5", "Maj7#5", "M7", "Dom7", "Maj7", "Dim7" });
}

TEST_CASE ("Melodic Minor uses the mMaj7 and Maj7#5 chord types")
{
    const Key key { 9, "Melodic Minor" };

    REQUIRE (triadNames (key) == Names { "Minor", "Minor", "Aug", "Major", "Major", "Dim", "Dim" });
    REQUIRE (seventhNames (key) == Names { "mMaj7", "M7", "Maj7#5", "Dom7", "Dom7", "M7b5", "M7b5" });
}

TEST_CASE ("In Dorian, degree 2 is Minor and degree 4 is Major")
{
    const Key key { 2, "Dorian" };

    REQUIRE (diatonicTriad (key, 2).chord.name == "Minor");
    REQUIRE (diatonicTriad (key, 4).chord.name == "Major");
}

TEST_CASE ("A diatonic chord has the root of its degree")
{
    REQUIRE (diatonicTriad (Key { 7, "Major" }, 1).root == 7);
    REQUIRE (diatonicTriad (Key { 7, "Major" }, 5).root == 2);
    REQUIRE (diatonicSeventh (Key { 7, "Major" }, 7).root == 6);
}

TEST_CASE ("Triads and 7th chords build for all 9 seven-note scales in all keys")
{
    int sevenNoteScales = 0;
    for (const auto& scale : bundledData ().scales.all ())
    {
        if (scale.steps.size () != 7)
            continue;

        ++sevenNoteScales;
        const Names triads   = triadNames (Key { 0, scale.name });
        const Names sevenths = seventhNames (Key { 0, scale.name });

        // The root does not change the chord types.
        for (PitchClass root = 1; root < 12; ++root)
        {
            CAPTURE (scale.name, root);
            REQUIRE (triadNames (Key { root, scale.name }) == triads);
            REQUIRE (seventhNames (Key { root, scale.name }) == sevenths);
        }
    }

    REQUIRE (sevenNoteScales == 9);
}

TEST_CASE ("A scale that does not have 7 notes throws std::invalid_argument")
{
    for (const auto& scale : bundledData ().scales.all ())
    {
        if (scale.steps.size () == 7)
            continue;

        CAPTURE (scale.name);
        REQUIRE_THROWS_AS (diatonicTriad (Key { 0, scale.name }, 1), std::invalid_argument);
        REQUIRE_THROWS_AS (diatonicSeventh (Key { 0, scale.name }, 1), std::invalid_argument);
    }
}

TEST_CASE ("A degree out of the range 1 to 7 throws std::invalid_argument")
{
    const Key key { 0, "Major" };

    REQUIRE_THROWS_AS (diatonicTriad (key, 0), std::invalid_argument);
    REQUIRE_THROWS_AS (diatonicTriad (key, 8), std::invalid_argument);
    REQUIRE_THROWS_AS (diatonicSeventh (key, -1), std::invalid_argument);
}

TEST_CASE ("A chord type that is not in the registry throws DataError with the intervals")
{
    DataSet data = loadBundledData ();
    data.chords  = ChordRegistry::fromJsonText (R"({ "schemaVersion": 1, "chords": [
        { "name": "Major", "symbol": "", "intervals": [4, 7], "optional": [], "tags": [] } ] })");

    const Key key { 0, "Major" };
    REQUIRE (diatonicTriad (key, 1, data).chord.name == "Major");

    try
    {
        diatonicTriad (key, 2, data);
        FAIL ("No error");
    }
    catch (const DataError& e)
    {
        REQUIRE (e.file == "chords.json");
        REQUIRE (e.message == "no chord type has the intervals 3, 7");
    }
}

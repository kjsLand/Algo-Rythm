#include "algo/Data.h"
#include "algo/DataError.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <functional>
#include <string>

using namespace algo;
using Catch::Matchers::ContainsSubstring;

namespace
{
    void requireDataError (const std::function<void ()>& load, const std::string& file, const std::string& entry, const std::string& problem)
    {
        try
        {
            load ();
            FAIL ("No error");
        }
        catch (const DataError& e)
        {
            REQUIRE (e.file == file);
            REQUIRE (e.entry == entry);
            REQUIRE_THAT (e.message, ContainsSubstring (problem));

            // The text of the error also gives all three.
            const std::string text = e.what ();
            REQUIRE_THAT (text, ContainsSubstring (file));
            REQUIRE_THAT (text, ContainsSubstring (entry));
            REQUIRE_THAT (text, ContainsSubstring (problem));
        }
    }

    void requireScaleError (const std::string& text, const std::string& entry, const std::string& problem)
    {
        requireDataError ([&]
        {
            ScaleRegistry::fromJsonText (text);
        }, "scales.json", entry, problem);
    }

    void requireStrumError (const std::string& entryJson, const std::string& entry, const std::string& problem)
    {
        const std::string text = R"({ "schemaVersion": 1, "strums": [ )" + entryJson + " ] }";
        requireDataError ([&]
        {
            parseStrums (text);
        }, "strums.json", entry, problem);
    }

    void requireShapeError (const std::string& entryJson, const std::string& entry, const std::string& problem)
    {
        const auto        chords = loadBundledData ().chords;
        const std::string text   = R"({ "schemaVersion": 1, "shapes": [ )" + entryJson + " ] }";
        requireDataError ([&]
        {
            parseShapes (text, chords);
        }, "shapes.json", entry, problem);
    }

    void requireProgressionError (const std::string& entryJson, const std::string& entry, const std::string& problem)
    {
        const auto        data = loadBundledData ();
        const std::string text = R"({ "schemaVersion": 1, "progressions": [ )" + entryJson + " ] }";
        requireDataError ([&]
        {
            ProgressionLibrary::fromJsonText (text, data.scales, data.chords);
        }, "progressions.json", entry, problem);
    }
}

TEST_CASE ("All bundled data files pass validation")
{
    const auto data = loadBundledData ();

    REQUIRE_FALSE (data.scales.all ().empty ());
    REQUIRE_FALSE (data.chords.all ().empty ());
    REQUIRE_FALSE (data.progressions.all ().empty ());
    REQUIRE_FALSE (data.strums.empty ());
    REQUIRE_FALSE (data.shapes.empty ());
}

TEST_CASE ("A file that is not valid JSON causes an error")
{
    requireScaleError ("{ not json", "", "not valid JSON");
    requireScaleError ("", "", "not valid JSON");
    requireScaleError ("[]", "", "must be an object");
}

TEST_CASE ("Each JSON document must contain schemaVersion")
{
    requireScaleError (R"({ "scales": [] })", "", "'schemaVersion' is missing");
    requireScaleError (R"({ "schemaVersion": "1", "scales": [] })", "", "must be an integer");
    requireScaleError (R"({ "schemaVersion": 2, "scales": [] })", "", "larger than the supported version");
}

TEST_CASE ("A data file must contain its list")
{
    requireScaleError (R"({ "schemaVersion": 1 })", "", "'scales' is missing");
    requireScaleError (R"({ "schemaVersion": 1, "scales": 5 })", "", "must be a list");
}

TEST_CASE ("An entry with no name is identified by its position")
{
    requireScaleError (R"({ "schemaVersion": 1, "scales": [ { "name": "A", "steps": [12], "tags": [] }, { "steps": [12] } ] })", "#2", "'name' is missing");
    requireScaleError (R"({ "schemaVersion": 1, "scales": [ { "name": 5 } ] })", "#1", "'name' must be a string");
    requireScaleError (R"({ "schemaVersion": 1, "scales": [ 5 ] })", "#1", "must be an object");
}

TEST_CASE ("A required field that is missing causes an error")
{
    requireScaleError (R"({ "schemaVersion": 1, "scales": [ { "name": "A", "tags": [] } ] })", "A", "'steps' is missing");

    requireStrumError (R"({ "name": "A", "slots": "D", "accents": [0], "difficulty": "Beginner", "tags": [] })", "A", "'steps' is missing");
    requireShapeError (R"({ "name": "A", "chord": "Major", "rootString": 6, "fingers": [0,0,0,0,0,0], "barre": false, "difficulty": "Beginner" })", "A", "'frets' is missing");
    requireProgressionError (R"({ "name": "A", "numerals": ["1"], "tags": [], "roles": ["loop"], "difficulty": "Beginner" })", "A", "'mode' is missing");
}

TEST_CASE ("A field with the wrong JSON type causes an error")
{
    requireScaleError (R"({ "schemaVersion": 1, "scales": [ { "name": "A", "steps": 12, "tags": [] } ] })", "A", "'steps' must be a list");
    requireScaleError (R"({ "schemaVersion": 1, "scales": [ { "name": "A", "steps": ["12"], "tags": [] } ] })", "A", "'steps' must contain only integers");
    requireScaleError (R"({ "schemaVersion": 1, "scales": [ { "name": "A", "steps": [12], "tags": [5] } ] })", "A", "'tags' must contain only strings");

    requireStrumError (R"({ "name": "A", "steps": "1", "slots": "D", "accents": [0], "difficulty": "Beginner", "tags": [] })", "A", "'steps' must be an integer");
    requireStrumError (R"({ "name": "A", "steps": 1, "slots": 5, "accents": [0], "difficulty": "Beginner", "tags": [] })", "A", "'slots' must be a string");
    requireStrumError (R"({ "name": "A", "steps": 1, "slots": "D", "accents": ["0"], "difficulty": "Beginner", "tags": [] })", "A", "'accents' must contain only numbers");
    requireShapeError (R"({ "name": "A", "chord": "Major", "rootString": 6, "frets": [0,0,0,0,0,0], "fingers": [0,0,0,0,0,0], "barre": "no", "difficulty": "Beginner" })", "A", "'barre' must be true or false");
    requireShapeError (R"({ "name": "A", "chord": "Major", "rootString": 6, "frets": [0,0,0,0,0,"y"], "fingers": [0,0,0,0,0,0], "barre": false, "difficulty": "Beginner" })", "A", "integer or");
}

TEST_CASE ("A name that is not unique in its file causes an error")
{
    requireScaleError (R"({ "schemaVersion": 1, "scales": [ { "name": "Same", "steps": [12], "tags": [] }, { "name": "same", "steps": [12], "tags": [] } ] })", "same", "not unique");
}

TEST_CASE ("A reference to an unknown scale or chord causes an error")
{
    requireProgressionError (R"({ "name": "A", "numerals": ["1"], "mode": "Klingon", "tags": [], "roles": ["loop"], "difficulty": "Beginner" })", "A", "'Klingon' is not in the scale registry");
    requireProgressionError (R"({ "name": "A", "numerals": ["1klingon"], "mode": "Major", "tags": [], "roles": ["loop"], "difficulty": "Beginner" })", "A", "unknown chord symbol 'klingon'");
    requireShapeError (R"({ "name": "A", "chord": "Klingon", "rootString": 6, "frets": [0,0,0,0,0,0], "fingers": [0,0,0,0,0,0], "barre": false, "difficulty": "Beginner" })", "A", "'Klingon' is not in the chord registry");
}

TEST_CASE ("A value that is not permitted causes an error")
{
    requireProgressionError (R"({ "name": "A", "numerals": ["8"], "mode": "Major", "tags": [], "roles": ["loop"], "difficulty": "Beginner" })", "A", "numeral '8' is not valid");
    requireProgressionError (R"({ "name": "A", "numerals": [], "mode": "Major", "tags": [], "roles": ["loop"], "difficulty": "Beginner" })", "A", "'numerals' must not be empty");
    requireProgressionError (R"({ "name": "A", "numerals": ["1"], "mode": "Major", "tags": [], "roles": ["intro"], "difficulty": "Beginner" })", "A", "role 'intro' is not valid");
    requireProgressionError (R"({ "name": "A", "numerals": ["1"], "mode": "Major", "tags": [], "roles": ["loop"], "difficulty": "Expert" })", "A", "unknown difficulty 'Expert'");

    requireStrumError (R"({ "name": "A", "steps": 2, "slots": "D", "accents": [0, 0], "difficulty": "Beginner", "tags": [] })", "A", "'steps' is 2");
    requireStrumError (R"({ "name": "A", "steps": 1, "slots": "D", "accents": [0, 0], "difficulty": "Beginner", "tags": [] })", "A", "'accents' has 2 values");
    requireStrumError (R"({ "name": "A", "steps": 2, "slots": "UD", "accents": [0, 0], "difficulty": "Beginner", "tags": [] })", "A", "slot 0 must be 'D'");
    requireStrumError (R"({ "name": "A", "steps": 2, "slots": "DQ", "accents": [0, 0], "difficulty": "Beginner", "tags": [] })", "A", "unknown stroke 'Q'");
    requireStrumError (R"({ "name": "A", "steps": 1, "slots": "D", "accents": [1.5], "difficulty": "Beginner", "tags": [] })", "A", "range 0 to 1");

    requireShapeError (R"({ "name": "A", "chord": "Major", "rootString": 7, "frets": [0,0,0,0,0,0], "fingers": [0,0,0,0,0,0], "barre": false, "difficulty": "Beginner" })", "A", "'rootString' must be in the range");
    requireShapeError (R"({ "name": "A", "chord": "Major", "rootString": 6, "frets": [0,0,0,0,0], "fingers": [0,0,0,0,0,0], "barre": false, "difficulty": "Beginner" })", "A", "'frets' must have 6 values");
    requireShapeError (R"({ "name": "A", "chord": "Major", "rootString": 6, "frets": [0,0,0,0,0,0], "fingers": [0,0,0,0,0,5], "barre": false, "difficulty": "Beginner" })", "A", "'fingers' must be in the range");
    requireShapeError (R"({ "name": "A", "chord": "Major", "rootString": 6, "frets": ["x",0,0,0,0,0], "fingers": [0,0,0,0,0,0], "barre": false, "difficulty": "Beginner" })", "A", "root string must not be muted");
    requireShapeError (R"({ "name": "A", "chord": "Major", "rootString": 6, "frets": [1,0,0,0,0,0], "fingers": [0,0,0,0,0,0], "barre": false, "difficulty": "Beginner" })", "A", "movable shape must have the value 0");
}

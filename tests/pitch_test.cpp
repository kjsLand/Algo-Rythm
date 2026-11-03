#include "algo/Data.h"
#include "algo/Pitch.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <map>
#include <set>
#include <stdexcept>
#include <string>

using namespace algo;

TEST_CASE ("parsePitchClass accepts sharps, flats and lower case")
{
    const std::map<std::string, PitchClass> names {
        { "C", 0 }, { "c", 0 }, { "C#", 1 }, { "c#", 1 }, { "Db", 1 }, { "db", 1 }, { "D", 2 }, { "Eb", 3 }, { "E", 4 }, { "Fb", 4 }, { "E#", 5 }, { "F", 5 }, { "F#", 6 }, { "Gb", 6 }, { "G", 7 }, { "Ab", 8 }, { "A", 9 }, { "a", 9 }, { "Bb", 10 }, { "bb", 10 }, { "B", 11 }, { "b", 11 }, { "Cb", 11 }, { "B#", 0 }, { "C##", 2 }, { "Dbb", 0 }
    };

    for (const auto& [name, pc] : names)
    {
        CAPTURE (name);
        REQUIRE (parsePitchClass (name) == pc);
    }
}

TEST_CASE ("parsePitchClass accepts an optional octave number")
{
    REQUIRE (parsePitchClass ("C4") == 0);
    REQUIRE (parsePitchClass ("F#3") == 6);
    REQUIRE (parsePitchClass ("C-1") == 0);
}

TEST_CASE ("parseMidiNote reads the octave number")
{
    REQUIRE (parseMidiNote ("C4") == 60);
    REQUIRE (parseMidiNote ("c4") == 60);
    REQUIRE (parseMidiNote ("A4") == 69);
    REQUIRE (parseMidiNote ("E2") == 40);
    REQUIRE (parseMidiNote ("C-1") == 0);
    REQUIRE (parseMidiNote ("G9") == 127);
}

TEST_CASE ("Accidentals keep the written octave")
{
    REQUIRE (parseMidiNote ("B#3") == 60);
    REQUIRE (parseMidiNote ("Cb4") == 59);
}

TEST_CASE ("A name that is not valid throws std::invalid_argument with the name")
{
    for (const char* name : { "", "H", "Cx", "C4x", "4", "#", "C-", "Bbx", "C100" })
    {
        CAPTURE (name);
        REQUIRE_THROWS_AS (parsePitchClass (name), std::invalid_argument);
        REQUIRE_THROWS_AS (parseMidiNote (name), std::invalid_argument);
    }

    REQUIRE_THROWS_WITH (parsePitchClass ("Hb"), Catch::Matchers::ContainsSubstring ("Hb"));
    REQUIRE_THROWS_WITH (parseMidiNote ("Hb4"), Catch::Matchers::ContainsSubstring ("Hb4"));
}

TEST_CASE ("parseMidiNote needs an octave number and a note in the MIDI range")
{
    REQUIRE_THROWS_AS (parseMidiNote ("C"), std::invalid_argument);
    REQUIRE_THROWS_AS (parseMidiNote ("G#9"), std::invalid_argument);
    REQUIRE_THROWS_AS (parseMidiNote ("Cb-1"), std::invalid_argument);
    REQUIRE_THROWS_AS (parseMidiNote ("C-2"), std::invalid_argument);
}

TEST_CASE ("usesFlats is true for the flat major keys and their relative minors")
{
    const std::set<PitchClass> flatMajor { 5, 10, 3, 8, 1, 6 };
    const std::set<PitchClass> flatMinor { 2, 7, 0, 5, 10, 3 };

    for (PitchClass root = 0; root < 12; ++root)
    {
        CAPTURE (root);
        REQUIRE (usesFlats (Key { root, "Major" }) == (flatMajor.count (root) > 0));
        REQUIRE (usesFlats (Key { root, "Natural Minor" }) == (flatMinor.count (root) > 0));
        REQUIRE (usesFlats (Key { root, "Harmonic Minor" }) == (flatMinor.count (root) > 0));
        REQUIRE (usesFlats (Key { root, "Melodic Minor" }) == (flatMinor.count (root) > 0));
    }
}

TEST_CASE ("usesFlats follows the parent major of a mode")
{
    REQUIRE (usesFlats (Key { parsePitchClass ("G"), "Dorian" }));           // F major
    REQUIRE (usesFlats (Key { parsePitchClass ("C"), "Mixolydian" }));       // F major
    REQUIRE (usesFlats (Key { parsePitchClass ("A"), "Phrygian" }));         // F major
    REQUIRE (usesFlats (Key { parsePitchClass ("Bb"), "Lydian" }));          // F major
    REQUIRE (usesFlats (Key { parsePitchClass ("E"), "Locrian" }));          // F major
    REQUIRE_FALSE (usesFlats (Key { parsePitchClass ("D"), "Dorian" }));     // C major
    REQUIRE_FALSE (usesFlats (Key { parsePitchClass ("E"), "Dorian" }));     // D major
    REQUIRE_FALSE (usesFlats (Key { parsePitchClass ("D"), "Mixolydian" })); // G major
}

TEST_CASE ("usesFlats does not depend on the case of the scale name")
{
    REQUIRE (usesFlats (Key { 2, "natural minor" }));
    REQUIRE (usesFlats (Key { 2, "NATURAL MINOR" }));
}

TEST_CASE ("pitchClassName uses flats in flat keys and sharps in sharp keys")
{
    REQUIRE (pitchClassName (10, Key { 5, "Major" }) == "Bb");
    REQUIRE (pitchClassName (10, Key { 7, "Major" }) == "A#");
    REQUIRE (pitchClassName (1, Key { 2, "Natural Minor" }) == "Db");
    REQUIRE (pitchClassName (1, Key { 9, "Natural Minor" }) == "C#");
    REQUIRE (pitchClassName (6, Key { 0, "Major" }) == "F#");
}

TEST_CASE ("pitchClassName gives the same name for a value out of the range 0 to 11")
{
    REQUIRE (pitchClassName (22, Key { 5, "Major" }) == "Bb");
    REQUIRE (pitchClassName (-2, Key { 5, "Major" }) == "Bb");
}

TEST_CASE ("Gb major and its relatives spell pitch class 11 as Cb")
{
    REQUIRE (pitchClassName (11, Key { 6, "Major" }) == "Cb");
    REQUIRE (pitchClassName (11, Key { 3, "Natural Minor" }) == "Cb");
    REQUIRE (pitchClassName (11, Key { 5, "Major" }) == "B");
    REQUIRE (midiNoteName (59, Key { 6, "Major" }) == "Cb4");
}

TEST_CASE ("midiNoteName adds the octave number")
{
    const Key cMajor { 0, "Major" };
    const Key fMajor { 5, "Major" };

    REQUIRE (midiNoteName (60, cMajor) == "C4");
    REQUIRE (midiNoteName (40, cMajor) == "E2");
    REQUIRE (midiNoteName (0, cMajor) == "C-1");
    REQUIRE (midiNoteName (127, cMajor) == "G9");
    REQUIRE (midiNoteName (61, cMajor) == "C#4");
    REQUIRE (midiNoteName (61, fMajor) == "Db4");
}

TEST_CASE ("midiNoteName parses back to the same note in each key")
{
    for (const auto& scale : bundledData ().scales.all ())
        for (PitchClass root = 0; root < 12; ++root)
            for (MidiNote note = 0; note <= 127; ++note)
            {
                const Key  key { root, scale.name };
                const auto name = midiNoteName (note, key);

                CAPTURE (scale.name, root, note, name);
                REQUIRE (parseMidiNote (name) == note);
                REQUIRE (parsePitchClass (pitchClassName (note, key)) == note % 12);
            }
}

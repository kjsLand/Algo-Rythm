#include "algo/Nashville.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <stdexcept>
#include <string>
#include <vector>

using namespace algo;

namespace
{
    std::string nameOf (const std::string& numeral, const Key& key)
    {
        return chordName (parseNumeral (numeral), key);
    }
}

TEST_CASE ("parseNumeral reads the degree, accidental, quality and bass")
{
    const auto plain = parseNumeral ("1");
    REQUIRE (plain.degree == 1);
    REQUIRE (plain.accidental == 0);
    REQUIRE (plain.quality == "diatonic");
    REQUIRE (plain.bassDegree == 0);
    REQUIRE (plain.bassAccidental == 0);

    const auto minor = parseNumeral ("6m");
    REQUIRE (minor.degree == 6);
    REQUIRE (minor.quality == "m");

    const auto flat = parseNumeral ("b7");
    REQUIRE (flat.degree == 7);
    REQUIRE (flat.accidental == -1);
    REQUIRE (flat.quality == "diatonic");

    REQUIRE (parseNumeral ("4maj7").quality == "maj7");

    const auto slash = parseNumeral ("5/7");
    REQUIRE (slash.degree == 5);
    REQUIRE (slash.quality == "diatonic");
    REQUIRE (slash.bassDegree == 7);

    const auto sharp = parseNumeral ("#4dim");
    REQUIRE (sharp.degree == 4);
    REQUIRE (sharp.accidental == 1);
    REQUIRE (sharp.quality == "dim");

    const auto flatBass = parseNumeral ("1m7/b3");
    REQUIRE (flatBass.quality == "m7");
    REQUIRE (flatBass.bassDegree == 3);
    REQUIRE (flatBass.bassAccidental == -1);
}

TEST_CASE ("parseNumeral treats the digit after the degree as a quality")
{
    const auto dominant = parseNumeral ("57");
    REQUIRE (dominant.degree == 5);
    REQUIRE (dominant.quality == "7");
}

TEST_CASE ("parseNumeral throws std::invalid_argument for a format that is not valid")
{
    for (const char* text : { "", "0", "8", "m", "bb7", "x1", "5/", "5/8", "5/m", "1/2/3", "1diatonic" })
    {
        CAPTURE (text);
        REQUIRE_THROWS_AS (parseNumeral (text), std::invalid_argument);
    }
}

TEST_CASE ("resolve replaces a diatonic quality with the triad on the degree")
{
    const Key cMajor { 0, "Major" };

    REQUIRE (resolve (parseNumeral ("1"), cMajor).quality == "");
    REQUIRE (resolve (parseNumeral ("6"), cMajor).quality == "m");
    REQUIRE (resolve (parseNumeral ("7"), cMajor).quality == "dim");
    REQUIRE (resolve (parseNumeral ("3"), Key { 9, "Natural Minor" }).quality == "");
}

TEST_CASE ("resolve does not change an explicit quality")
{
    REQUIRE (resolve (parseNumeral ("4m"), Key { 0, "Major" }).quality == "m");
    REQUIRE (resolve (parseNumeral ("5maj7"), Key { 0, "Major" }).quality == "maj7");
}

TEST_CASE ("resolve makes a major chord on a degree with an accidental")
{
    REQUIRE (resolve (parseNumeral ("b7"), Key { 0, "Major" }).quality == "");
    REQUIRE (resolve (parseNumeral ("#4"), Key { 0, "Major" }).quality == "");
}

TEST_CASE ("resolve throws std::invalid_argument for an unknown quality")
{
    REQUIRE_THROWS_AS (resolve (parseNumeral ("1xyz"), Key { 0, "Major" }), std::invalid_argument);
}

TEST_CASE ("chordRootPc applies the accidental to the scale degree")
{
    const Key cMajor { 0, "Major" };

    REQUIRE (chordRootPc (parseNumeral ("1"), cMajor) == 0);
    REQUIRE (chordRootPc (parseNumeral ("5"), cMajor) == 7);
    REQUIRE (chordRootPc (parseNumeral ("b7"), cMajor) == 10);
    REQUIRE (chordRootPc (parseNumeral ("#4"), cMajor) == 6);
    REQUIRE (chordRootPc (parseNumeral ("b1"), cMajor) == 11);
    REQUIRE (chordRootPc (parseNumeral ("7"), Key { 9, "Natural Minor" }) == 7);
}

TEST_CASE ("A key with no 7-note scale throws std::invalid_argument")
{
    const Key key { 9, "Minor Pentatonic" };

    REQUIRE_THROWS_AS (chordRootPc (parseNumeral ("1"), key), std::invalid_argument);
    REQUIRE_THROWS_AS (resolve (parseNumeral ("1"), key), std::invalid_argument);
    REQUIRE_THROWS_AS (chordName (parseNumeral ("1m"), key), std::invalid_argument);
}

TEST_CASE ("chordName gives the root, the quality and the bass")
{
    REQUIRE (nameOf ("6m", Key { 7, "Major" }) == "Em");
    REQUIRE (nameOf ("b7", Key { 0, "Major" }) == "Bb");
    REQUIRE (nameOf ("5/7", Key { 7, "Major" }) == "D/F#");
    REQUIRE (nameOf ("4maj7", Key { 0, "Major" }) == "Fmaj7");
    REQUIRE (nameOf ("#4dim", Key { 0, "Major" }) == "F#dim");
    REQUIRE (nameOf ("57", Key { 0, "Major" }) == "G7");
    REQUIRE (nameOf ("7", Key { 0, "Major" }) == "Bdim");
    REQUIRE (nameOf ("7", Key { 9, "Natural Minor" }) == "G");
    REQUIRE (nameOf ("1m", Key { 2, "Natural Minor" }) == "Dm");
}

TEST_CASE ("A note with an accidental follows the accidental and not the key")
{
    REQUIRE (nameOf ("b3", Key { 7, "Major" }) == "Bb"); // A sharp key
    REQUIRE (nameOf ("#5", Key { 5, "Major" }) == "C#"); // A flat key
    REQUIRE (nameOf ("1/b7", Key { 0, "Major" }) == "C/Bb");
    REQUIRE (nameOf ("1/#4", Key { 5, "Major" }) == "F/B");
}

TEST_CASE ("Note names in a chord name use the spelling of the key")
{
    REQUIRE (nameOf ("4", Key { 5, "Major" }) == "Bb");
    REQUIRE (nameOf ("3", Key { 1, "Major" }) == "Fm");
    REQUIRE (nameOf ("5/7", Key { 3, "Major" }) == "Bb/D");
}

TEST_CASE ("1 5 6 4 gives the correct chord names in all 12 major keys")
{
    const std::array<std::vector<std::string>, 12> expected {
        std::vector<std::string> { "C", "G", "Am", "F" },
        { "Db", "Ab", "Bbm", "Gb" },
        { "D", "A", "Bm", "G" },
        { "Eb", "Bb", "Cm", "Ab" },
        { "E", "B", "C#m", "A" },
        { "F", "C", "Dm", "Bb" },
        { "Gb", "Db", "Ebm", "Cb" },
        { "G", "D", "Em", "C" },
        { "Ab", "Eb", "Fm", "Db" },
        { "A", "E", "F#m", "D" },
        { "Bb", "F", "Gm", "Eb" },
        { "B", "F#", "G#m", "E" },
    };

    for (PitchClass root = 0; root < 12; ++root)
    {
        CAPTURE (root);

        const Key                key { root, "Major" };
        std::vector<std::string> names;
        for (const char* numeral : { "1", "5", "6", "4" })
            names.push_back (nameOf (numeral, key));

        REQUIRE (names == expected[static_cast<std::size_t> (root)]);
    }
}

TEST_CASE ("numeralText gives the numeral back")
{
    for (const char* text : { "1", "6m", "b7", "4maj7", "5/7", "#4dim", "57", "1sus2", "1m7/b3", "1/#4" })
    {
        CAPTURE (text);
        REQUIRE (numeralText (parseNumeral (text)) == text);
    }
}

TEST_CASE ("numeralText writes the quality of a resolved chord")
{
    REQUIRE (numeralText (resolve (parseNumeral ("6"), Key { 0, "Major" })) == "6m");
    REQUIRE (numeralText (resolve (parseNumeral ("1"), Key { 0, "Major" })) == "1");
}

TEST_CASE ("Each bundled progression gives chord names in all 12 keys of its mode")
{
    for (const auto& progression : bundledData ().progressions.all ())
        for (const auto& numeral : progression.numerals)
            for (PitchClass root = 0; root < 12; ++root)
            {
                CAPTURE (progression.name, numeral, root);

                const auto symbol = parseNumeral (numeral);
                REQUIRE (numeralText (symbol) == numeral);
                REQUIRE_FALSE (chordName (symbol, Key { root, progression.mode }).empty ());
            }
}

#include "algo/Data.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <regex>
#include <string>
#include <vector>

using namespace algo;

namespace
{
    struct Numeral
    {
        int         accidental { 0 };
        int         degree { 1 };
        std::string quality;
        bool        hasBass { false };
    };

    Numeral parse (const std::string& text)
    {
        static const std::regex format ("([b#]?)([1-7])([^/]*)(/.*)?");

        std::smatch match;
        REQUIRE (std::regex_match (text, match, format));

        Numeral numeral;
        numeral.accidental = match[1] == "b" ? -1 : (match[1] == "#" ? 1 : 0);
        numeral.degree     = std::stoi (match[2]);
        numeral.quality    = match[3];
        numeral.hasBass    = match[4].matched;
        return numeral;
    }

    // The semitones above the tonic of the root of this numeral.
    int rootInterval (const Numeral& numeral, const std::vector<PitchClass>& scale)
    {
        return (scale[static_cast<std::size_t> (numeral.degree - 1)] + numeral.accidental + 12) % 12;
    }

    // The semitones from the root up to a scale note, `offset` degrees above the root.
    int degreeInterval (const std::vector<PitchClass>& scale, int degree, int offset)
    {
        const auto root = scale[static_cast<std::size_t> (degree - 1)];
        const auto note = scale[static_cast<std::size_t> ((degree - 1 + offset) % 7)];
        return (note - root + 12) % 12;
    }

    // Returns the symbol of the triad on this degree. Returns "other" if it is not major or minor.
    std::string diatonicTriad (const std::vector<PitchClass>& scale, int degree)
    {
        const int third = degreeInterval (scale, degree, 2);
        const int fifth = degreeInterval (scale, degree, 4);
        if (third == 4 && fifth == 7) return "";
        if (third == 3 && fifth == 7) return "m";
        return "other";
    }

    std::string diatonicSeventh (const std::vector<PitchClass>& scale, int degree)
    {
        const std::string triad   = diatonicTriad (scale, degree);
        const int         seventh = degreeInterval (scale, degree, 6);
        if (triad == "" && seventh == 11) return "maj7";
        if (triad == "" && seventh == 10) return "7";
        if (triad == "m" && seventh == 10) return "m7";
        return "other";
    }

    bool hasItem (const std::vector<std::string>& items, const std::string& item)
    {
        return std::find (items.begin (), items.end (), item) != items.end ();
    }
}

TEST_CASE ("The file contains 20 or more 4-chord progressions")
{
    const auto data = loadBundledData ();

    REQUIRE (data.progressions.all ().size () >= 20);

    // REQ-06-2: each section has a loop of 4 chords.
    for (const auto& progression : data.progressions.all ())
    {
        CAPTURE (progression.name);
        REQUIRE (progression.numerals.size () == 4);
    }
}

TEST_CASE ("A filter for chorus and Beginner in a major key returns 5 or more progressions")
{
    ProgressionFilter filter;
    filter.mode       = "Major";
    filter.role       = "chorus";
    filter.difficulty = Difficulty::Beginner;

    const auto matches = loadBundledData ().progressions.filter (filter);

    REQUIRE (matches.size () >= 5);
    for (const auto& progression : matches)
    {
        CAPTURE (progression.name);
        REQUIRE (progression.mode == "Major");
        REQUIRE (hasItem (progression.roles, "chorus"));
        REQUIRE (progression.difficulty == Difficulty::Beginner);
    }
}

TEST_CASE ("An empty filter returns all progressions in file order")
{
    const auto library = loadBundledData ().progressions;

    const auto matches = library.filter ({});

    REQUIRE (matches.size () == library.all ().size ());
    REQUIRE (matches.front ().name == "Axis");
    REQUIRE (matches.back ().name == library.all ().back ().name);
}

TEST_CASE ("The filter checks each field")
{
    const auto library = loadBundledData ().progressions;

    SECTION ("Mode, with no change for case")
    {
        ProgressionFilter filter;
        filter.mode = "dorian";
        REQUIRE (library.filter (filter).size () == 2);
    }

    SECTION ("Tag")
    {
        ProgressionFilter filter;
        filter.tag = "borrowed";
        for (const auto& progression : library.filter (filter))
            REQUIRE (hasItem (progression.tags, "borrowed"));
        REQUIRE_FALSE (library.filter (filter).empty ());
    }

    SECTION ("A difficulty also matches lower difficulties")
    {
        ProgressionFilter beginner;
        beginner.difficulty = Difficulty::Beginner;
        ProgressionFilter advanced;
        advanced.difficulty = Difficulty::Advanced;

        REQUIRE (library.filter (beginner).size () < library.filter (advanced).size ());
        REQUIRE (library.filter (advanced).size () == library.all ().size ());
    }

    SECTION ("No match")
    {
        ProgressionFilter filter;
        filter.mode = "Klingon";
        REQUIRE (library.filter (filter).empty ());
    }
}

TEST_CASE ("Each mode has a Beginner progression for each role")
{
    const auto library = loadBundledData ().progressions;

    for (const char* mode : { "Major", "Natural Minor", "Dorian", "Mixolydian" })
    {
        for (const char* role : { "verse", "chorus", "bridge", "loop" })
        {
            ProgressionFilter filter;
            filter.mode       = mode;
            filter.role       = role;
            filter.difficulty = Difficulty::Beginner;

            CAPTURE (mode, role);
            REQUIRE_FALSE (library.filter (filter).empty ());
        }
    }
}

TEST_CASE ("Beginner progressions contain only diatonic major and minor triads")
{
    const auto data = loadBundledData ();

    for (const auto& progression : data.progressions.all ())
    {
        if (progression.difficulty != Difficulty::Beginner)
            continue;

        const auto scale = ScaleRegistry::pitchClasses (0, data.scales.get (progression.mode));
        for (const auto& text : progression.numerals)
        {
            CAPTURE (progression.name, text);
            const auto numeral = parse (text);
            const auto triad   = diatonicTriad (scale, numeral.degree);

            REQUIRE (numeral.accidental == 0);
            REQUIRE_FALSE (numeral.hasBass);
            REQUIRE (triad != "other");
            REQUIRE ((numeral.quality.empty () || numeral.quality == triad));
        }
    }
}

TEST_CASE ("A chord that is not diatonic has the tag borrowed or secondary")
{
    const auto data = loadBundledData ();

    for (const auto& progression : data.progressions.all ())
    {
        const auto scale = ScaleRegistry::pitchClasses (0, data.scales.get (progression.mode));
        for (const auto& text : progression.numerals)
        {
            const auto numeral = parse (text);

            bool diatonic = numeral.accidental == 0;
            if (numeral.quality == "m")
                diatonic = diatonic && diatonicTriad (scale, numeral.degree) == "m";
            if (numeral.quality == "7" || numeral.quality == "m7" || numeral.quality == "maj7")
                diatonic = diatonic && diatonicSeventh (scale, numeral.degree) == numeral.quality;

            CAPTURE (progression.name, text);
            REQUIRE ((diatonic || hasItem (progression.tags, "borrowed") || hasItem (progression.tags, "secondary")));
        }
    }
}

TEST_CASE ("Each progression ends on a chord that leads back to its first chord")
{
    const auto data = loadBundledData ();

    for (const auto& progression : data.progressions.all ())
    {
        const auto scale    = ScaleRegistry::pitchClasses (0, data.scales.get (progression.mode));
        const auto first    = parse (progression.numerals.front ());
        const auto last     = parse (progression.numerals.back ());
        const int  interval = rootInterval (last, scale);

        // REQ-06-4: 5 to 1, 4 to 1, b7 to 1, 4 to 6 and 5 to 6.
        const bool toOne = first.degree == 1 && (interval == 7 || interval == 5 || interval == 10);
        const bool toSix = first.degree == 6 && (interval == 7 || interval == 5);

        CAPTURE (progression.name);
        REQUIRE ((toOne || toSix));

        // T06-06: a bridge ends on 5.
        if (hasItem (progression.roles, "bridge"))
            REQUIRE (last.degree == 5);
    }
}

#include "algo/Fretboard.h"
#include "algo/Pitch.h"

#include <catch2/catch_test_macros.hpp>

#include <utility>
#include <vector>

using algo::Fretboard;

namespace
{
    using Positions = std::vector<std::pair<int, int>>;

    Positions search (const Fretboard& fb, const std::string& name)
    {
        Positions out;
        for (const auto& p : fb.findNoteByName (name))
            out.push_back ({ p.string, p.fret });
        return out;
    }

    // Every position on the neck with this pitch class.
    Positions positionsOfPitchClass (const Fretboard& fb, int pc)
    {
        Positions out;
        for (int s = 0; s < algo::kNumStrings; ++s)
            for (int f = 0; f < fb.numFrets (); ++f)
                if (fb.midiNoteAt (s, f) % 12 == pc)
                    out.push_back ({ s, f });
        return out;
    }
}

TEST_CASE ("A name without an octave matches only its pitch class")
{
    Fretboard fb;

    const auto results = search (fb, "C");

    REQUIRE_FALSE (results.empty ());
    for (const auto& [s, f] : results)
        REQUIRE (fb.midiNoteAt (s, f) % 12 == 0);

    REQUIRE (results == positionsOfPitchClass (fb, 0));
}

TEST_CASE ("A name with an octave matches only that MIDI note")
{
    Fretboard fb;

    const auto results = search (fb, "C4");

    REQUIRE_FALSE (results.empty ());
    for (const auto& [s, f] : results)
        REQUIRE (fb.midiNoteAt (s, f) == 60);
}

TEST_CASE ("Enharmonic names return the same positions")
{
    Fretboard fb;

    REQUIRE (search (fb, "Db") == search (fb, "C#"));
    REQUIRE (search (fb, "Db4") == search (fb, "C#4"));
    REQUIRE (search (fb, "Bb") == search (fb, "A#"));
    REQUIRE_FALSE (search (fb, "Db").empty ());
}

TEST_CASE ("Accidentals that cross an octave keep the written pitch")
{
    Fretboard fb;

    REQUIRE (search (fb, "B#3") == search (fb, "C4")); // MIDI 60
    REQUIRE (search (fb, "Cb4") == search (fb, "B3")); // MIDI 59
    REQUIRE_FALSE (search (fb, "B3").empty ());
}

TEST_CASE ("Note names are not case sensitive")
{
    Fretboard fb;

    REQUIRE (search (fb, "c") == search (fb, "C"));
    REQUIRE (search (fb, "c4") == search (fb, "C4"));
    REQUIRE (search (fb, "bb") == search (fb, "Bb"));
}

TEST_CASE ("A malformed name matches nothing")
{
    Fretboard fb;

    for (const char* name : { "", "H", "Cx", "C4x", "4", "#", "C-", "C99" })
    {
        CAPTURE (name);
        REQUIRE (search (fb, name).empty ());
    }
}

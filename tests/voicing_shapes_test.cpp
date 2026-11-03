#include "algo/Voicing.h"

#include "support/VoicingHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <set>
#include <string>

using namespace algo;
using algo::test::chordOf;
using algo::test::hasRule;
using algo::test::hasVoicing;
using algo::test::textOf;

TEST_CASE ("G Major gives the open G shape and the E-shape barre at fret 3")
{
    const auto candidates = shapeCandidates (chordOf (7, "Major"), Fretboard {});

    REQUIRE (hasVoicing (candidates, "320003"));
    REQUIRE (hasVoicing (candidates, "355433"));
}

TEST_CASE ("Each shape candidate gives the correct pitch classes")
{
    const Fretboard fretboard;

    for (PitchClass root = 0; root < 12; ++root)
        for (const auto& definition : bundledData ().chords.all ())
        {
            const auto chord = chordOf (root, definition.name);
            for (const auto& voicing : shapeCandidates (chord, fretboard))
            {
                CAPTURE (chord.name, textOf (voicing));
                const auto broken = violations (voicing, chord, Difficulty::Advanced, fretboard, false);
                REQUIRE_FALSE (hasRule (broken, kRuleChordTones));
                REQUIRE_FALSE (hasRule (broken, kRuleOtherNote));
                REQUIRE_FALSE (hasRule (broken, kRuleLowestNote));
            }
        }
}

TEST_CASE ("A shape candidate has no fret above 15 and no fret below the neck")
{
    // F# Major: the E-shape at fret 14 has a highest fret of 16.
    const auto candidates = shapeCandidates (chordOf (6, "Major"), Fretboard {});

    REQUIRE (hasVoicing (candidates, "244322"));
    REQUIRE_FALSE (candidates.empty ());

    for (const auto& voicing : candidates)
    {
        CAPTURE (textOf (voicing));
        REQUIRE (*std::max_element (voicing.frets.begin (), voicing.frets.end ()) <= kMaxCandidateFret);
    }

    // The triad on strings 3-5 needs 3 frets below its root, so C at fret 3 fits and it is the first one.
    const auto c = shapeCandidates (chordOf (0, "Major"), Fretboard {});
    REQUIRE (hasVoicing (c, "x320xx"));
}

TEST_CASE ("An open shape is used only if its root is the chord root")
{
    REQUIRE (hasVoicing (shapeCandidates (chordOf (0, "Major"), Fretboard {}), "x32010"));
    REQUIRE_FALSE (hasVoicing (shapeCandidates (chordOf (2, "Major"), Fretboard {}), "x32010"));
    REQUIRE (hasVoicing (shapeCandidates (chordOf (2, "Major"), Fretboard {}), "xx0232"));
}

TEST_CASE ("A capo moves the root of an open shape")
{
    // With a capo at fret 2, the open G shape sounds as A.
    const Fretboard capo (Tuning {}, 2);

    REQUIRE (hasVoicing (shapeCandidates (chordOf (9, "Major"), capo), "320003"));
    REQUIRE_FALSE (hasVoicing (shapeCandidates (chordOf (7, "Major"), capo), "320003"));
}

TEST_CASE ("A movable shape at fret 0 does not repeat an open shape")
{
    const auto candidates = shapeCandidates (chordOf (4, "Major"), Fretboard {});

    REQUIRE (std::count_if (candidates.begin (), candidates.end (), [] (const Voicing& voicing)
    {
        return textOf (voicing) == "022100";
    }) == 1);
}

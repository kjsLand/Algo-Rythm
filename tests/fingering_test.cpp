#include "algo/Voicing.h"

#include "support/VoicingHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>

using namespace algo;
using algo::test::chordOf;
using algo::test::textOf;
using algo::test::voicingOf;

namespace
{
    using Fingers = std::array<int, kNumStrings>;

    Fingers fingersOf (const char* text)
    {
        const auto result = assignFingers (voicingOf (text));
        REQUIRE (result.has_value ());
        REQUIRE (result->frets == voicingOf (text).frets);
        return result->fingers;
    }
}

TEST_CASE ("The open C voicing gets finger 3, finger 2 and finger 1")
{
    REQUIRE (fingersOf ("x32010") == Fingers { 0, 3, 2, 0, 1, 0 });
}

TEST_CASE ("The F barre gets finger 1 on all strings at the barre fret")
{
    REQUIRE (fingersOf ("133211") == Fingers { 1, 3, 4, 2, 1, 1 });
}

TEST_CASE ("A note at a lower fret gets a lower finger")
{
    REQUIRE (fingersOf ("320003") == Fingers { 2, 1, 0, 0, 0, 3 });
    REQUIRE (fingersOf ("x21202") == Fingers { 0, 2, 1, 3, 0, 4 });
}

TEST_CASE ("A barre can have other notes between its strings")
{
    const auto barre = findBarre (voicingOf ("x35553"));

    REQUIRE (barre.has_value ());
    REQUIRE (barre->fret == 3);
    REQUIRE (barre->firstString == 1);
    REQUIRE (barre->lastString == 5);

    REQUIRE (fingersOf ("x35553") == Fingers { 0, 1, 2, 3, 4, 1 });
}

TEST_CASE ("A barre is at the lowest fret and has 2 or more notes")
{
    REQUIRE_FALSE (findBarre (voicingOf ("x32010")).has_value ());
    REQUIRE_FALSE (findBarre (voicingOf ("000000")).has_value ());
    REQUIRE_FALSE (findBarre (Voicing {}).has_value ());

    // The notes at fret 3 are not at the lowest fret.
    REQUIRE_FALSE (findBarre (voicingOf ("x32333")).has_value ());
}

TEST_CASE ("A barre cannot cover an open or a muted string")
{
    REQUIRE_FALSE (findBarre (voicingOf ("x02020")).has_value ());
    REQUIRE_FALSE (findBarre (voicingOf ("2x2xxx")).has_value ());

    // The two notes on the high strings are a barre. The note on string 6 needs its own finger.
    const auto barre = findBarre (voicingOf ("20x022"));
    REQUIRE (barre.has_value ());
    REQUIRE (barre->firstString == 4);
    REQUIRE (barre->lastString == 5);

    REQUIRE (fingersOf ("20x022") == Fingers { 2, 0, 0, 0, 1, 1 });
    REQUIRE (fingerCount (voicingOf ("20x022")) == 2);
}

TEST_CASE ("A note that needs a fifth finger makes the voicing not valid")
{
    REQUIRE_FALSE (assignFingers (voicingOf ("x32333")).has_value ());
    REQUIRE (fingerCount (voicingOf ("x32333")) == 5);

    REQUIRE_FALSE (assignFingers (voicingOf ("123423")).has_value ());
}

TEST_CASE ("The old fingers are replaced")
{
    auto voicing       = voicingOf ("x32010");
    voicing.fingers[0] = 4;

    REQUIRE (assignFingers (voicing)->fingers == Fingers { 0, 3, 2, 0, 1, 0 });
}

TEST_CASE ("Each shape candidate that passes the filter has fingers")
{
    const Fretboard fretboard;

    for (PitchClass root = 0; root < 12; ++root)
        for (const auto& definition : bundledData ().chords.all ())
        {
            const auto chord = chordOf (root, definition.name);
            for (const auto& voicing : shapeCandidates (chord, fretboard))
            {
                if (!violations (voicing, chord, Difficulty::Advanced, fretboard, false).empty ())
                    continue;

                CAPTURE (chord.name, textOf (voicing));
                REQUIRE (assignFingers (voicing).has_value ());
            }
        }
}

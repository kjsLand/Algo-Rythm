#include "algo/Voicing.h"

#include "support/VoicingHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <string>

using namespace algo;
using algo::test::chordOf;
using algo::test::hasVoicing;
using algo::test::textOf;

namespace
{
    bool hasMuteBetween (const Voicing& voicing)
    {
        bool sounded = false;
        bool muted   = false;
        for (int fret : voicing.frets)
        {
            if (fret == kMutedString)
                muted = sounded;
            else if (muted)
                return true;
            else
                sounded = true;
        }
        return false;
    }
}

TEST_CASE ("The search for C Major includes the open C voicing")
{
    REQUIRE (hasVoicing (searchCandidates (chordOf (0, "Major"), Fretboard {}, Difficulty::Beginner), "x32010"));
}

TEST_CASE ("The search removes duplicate candidates")
{
    const auto candidates = searchCandidates (chordOf (0, "Major"), Fretboard {}, Difficulty::Advanced);

    std::set<std::string> texts;
    for (const auto& voicing : candidates)
        texts.insert (textOf (voicing));

    REQUIRE (texts.size () == candidates.size ());
}

TEST_CASE ("Each search candidate passes the filter")
{
    const Fretboard fretboard;

    for (const auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
        for (PitchClass root = 0; root < 12; ++root)
            for (const char* type : { "Major", "Minor", "Dom7", "Add9", "Dim" })
            {
                const auto chord = chordOf (root, type);
                for (const auto& voicing : searchCandidates (chord, fretboard, difficulty))
                {
                    CAPTURE (chord.name, textOf (voicing));
                    REQUIRE (violations (voicing, chord, difficulty, fretboard).empty ());
                }
            }
}

TEST_CASE ("Only an Advanced search has a muted string between two strings that sound")
{
    const auto chord    = chordOf (7, "Major");
    const auto beginner = searchCandidates (chord, Fretboard {}, Difficulty::Beginner);
    const auto advanced = searchCandidates (chord, Fretboard {}, Difficulty::Advanced);

    for (const auto& voicing : beginner)
    {
        CAPTURE (textOf (voicing));
        REQUIRE_FALSE (hasMuteBetween (voicing));
    }

    REQUIRE (hasVoicing (advanced, "3x0003"));
    REQUIRE_FALSE (hasVoicing (beginner, "3x0003"));
}

TEST_CASE ("The search puts a slash bass note at the bottom")
{
    // D/F#
    const auto candidates = searchCandidates (chordOf (2, "Major", 6), Fretboard {}, Difficulty::Beginner);

    REQUIRE (hasVoicing (candidates, "200232"));
    REQUIRE_FALSE (hasVoicing (candidates, "xx0232"));
}

TEST_CASE ("The search uses the capo")
{
    // With a capo at fret 2, x32010 sounds as D.
    const Fretboard capo (Tuning {}, 2);

    REQUIRE (hasVoicing (searchCandidates (chordOf (2, "Major"), capo, Difficulty::Beginner), "x32010"));
    REQUIRE_FALSE (hasVoicing (searchCandidates (chordOf (0, "Major"), capo, Difficulty::Beginner), "x32010"));
}

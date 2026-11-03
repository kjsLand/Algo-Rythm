#include "algo/Pitch.h"
#include "algo/Voicing.h"

#include "support/VoicingHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

using namespace algo;
using algo::test::chordOf;
using algo::test::hasRule;
using algo::test::textOf;
using algo::test::voicingOf;

namespace
{
    constexpr int kMaxLegacyFret = 12;

    // A copy of the old buildTightVoicing. It stays here after T07-08 deletes the function.
    std::vector<FretPos> legacyTightVoicing (const Fretboard& board, const FretPos& rootPos, const ChordDefinition& definition)
    {
        const int rootPc = pitchClassOf (board.midiNoteAt (rootPos.string, rootPos.fret));

        std::vector<FretPos> positions { rootPos };
        for (int interval : definition.intervals)
        {
            int     bestDistance = INT_MAX;
            FretPos best;

            for (int s = 0; s < kNumStrings; ++s)
                for (int f = 0; f <= kMaxLegacyFret; ++f)
                {
                    if (pitchClassOf (board.midiNoteAt (s, f)) != pitchClassOf (rootPc + interval))
                        continue;

                    const int distance = std::abs (f - rootPos.fret);
                    if (distance < bestDistance)
                    {
                        bestDistance = distance;
                        best         = { s, f };
                    }
                }
            positions.push_back (best);
        }
        return positions;
    }
}

TEST_CASE ("REQ-07-1: each string has one note or no note")
{
    const auto chord = chordOf (0, "Major");

    auto bad     = voicingOf ("x32010");
    bad.frets[1] = 40; // Not on the neck

    REQUIRE (violations (voicingOf ("x32010"), chord, Difficulty::Beginner).empty ());
    REQUIRE (violations (bad, chord, Difficulty::Beginner) == std::vector<std::string> { kRuleOneNotePerString });

    bad.frets[1] = -2;
    REQUIRE (violations (bad, chord, Difficulty::Beginner) == std::vector<std::string> { kRuleOneNotePerString });
}

TEST_CASE ("REQ-07-2: the lowest note is the root or the slash bass note")
{
    const auto c      = chordOf (0, "Major");
    const auto cOverE = chordOf (0, "Major", 4);

    REQUIRE_FALSE (hasRule (violations (voicingOf ("x32010"), c, Difficulty::Beginner), kRuleLowestNote));
    REQUIRE (hasRule (violations (voicingOf ("332010"), c, Difficulty::Beginner), kRuleLowestNote));

    REQUIRE_FALSE (hasRule (violations (voicingOf ("032010"), cOverE, Difficulty::Beginner), kRuleLowestNote));
    REQUIRE (hasRule (violations (voicingOf ("x32010"), cOverE, Difficulty::Beginner), kRuleLowestNote));
}

TEST_CASE ("REQ-07-2: the lowest note is the lowest pitch and not the lowest string")
{
    // The 12th fret of the low E string (E3) is above the open A string (A2).
    auto voicing     = voicingOf ("x02220");
    voicing.frets[0] = 12;

    REQUIRE_FALSE (hasRule (violations (voicing, chordOf (9, "Major"), Difficulty::Advanced), kRuleLowestNote));
}

TEST_CASE ("REQ-07-3: the voicing has the root, the 3rd and the 7th")
{
    const auto dom7 = chordOf (7, "Dom7");

    // The 5th is optional: G B F G with no D.
    REQUIRE_FALSE (hasRule (violations (voicingOf ("3x343x"), dom7, Difficulty::Advanced), kRuleChordTones));

    // G D G B: no 7th.
    REQUIRE (hasRule (violations (voicingOf ("320003"), dom7, Difficulty::Beginner), kRuleChordTones));

    // G D G with no 3rd.
    REQUIRE (hasRule (violations (voicingOf ("355xxx"), chordOf (7, "Major"), Difficulty::Intermediate, Fretboard {}, false), kRuleChordTones));
}

TEST_CASE ("REQ-07-4: the fret span depends on the difficulty")
{
    const auto am = chordOf (9, "Minor");

    // The open strings do not count: frets 1 and 5 are 4 frets apart.
    const auto wide = voicingOf ("x05515");

    REQUIRE (hasRule (violations (wide, am, Difficulty::Beginner), kRuleFretSpan));
    REQUIRE_FALSE (hasRule (violations (wide, am, Difficulty::Intermediate), kRuleFretSpan));

    REQUIRE (hasRule (violations (voicingOf ("x06516"), am, Difficulty::Advanced), kRuleFretSpan));
}

TEST_CASE ("REQ-07-5: a voicing uses 4 fingers or fewer and a barre counts as one finger")
{
    REQUIRE_FALSE (hasRule (violations (voicingOf ("133211"), chordOf (5, "Major"), Difficulty::Beginner), kRuleFingerCount));

    // C9 with five fretted notes. The notes at fret 3 are not at the lowest fret, so they have no barre.
    REQUIRE (hasRule (violations (voicingOf ("x32333"), chordOf (0, "Dom9"), Difficulty::Advanced), kRuleFingerCount));
}

TEST_CASE ("REQ-07-6: a strummed voicing has 4 or more strings")
{
    const auto g = chordOf (7, "Power");

    REQUIRE (hasRule (violations (voicingOf ("355xxx"), g, Difficulty::Intermediate), kRuleStringCount));
    REQUIRE_FALSE (hasRule (violations (voicingOf ("355xxx"), g, Difficulty::Intermediate, Fretboard {}, false), kRuleStringCount));
    REQUIRE_FALSE (hasRule (violations (voicingOf ("x32010"), chordOf (0, "Major"), Difficulty::Beginner), kRuleStringCount));
}

TEST_CASE ("REQ-07-7: only an Advanced voicing has a muted string between two strings that sound")
{
    const auto g = chordOf (7, "Major");

    REQUIRE (hasRule (violations (voicingOf ("3x0003"), g, Difficulty::Beginner), kRuleMutedString));
    REQUIRE (hasRule (violations (voicingOf ("3x0003"), g, Difficulty::Intermediate), kRuleMutedString));
    REQUIRE_FALSE (hasRule (violations (voicingOf ("3x0003"), g, Difficulty::Advanced), kRuleMutedString));
    REQUIRE_FALSE (hasRule (violations (voicingOf ("320003"), g, Difficulty::Beginner), kRuleMutedString));
}

TEST_CASE ("A note that is not in the chord breaks a rule")
{
    const auto c = chordOf (0, "Major");

    REQUIRE (hasRule (violations (voicingOf ("x32000"), c, Difficulty::Beginner), kRuleOtherNote));
    REQUIRE_FALSE (hasRule (violations (voicingOf ("x32010"), c, Difficulty::Beginner), kRuleOtherNote));

    // A slash bass note that is not in the chord is permitted: C/Bb.
    REQUIRE_FALSE (hasRule (violations (voicingOf ("x12010"), chordOf (0, "Major", 10), Difficulty::Beginner), kRuleOtherNote));
}

TEST_CASE ("A voicing with no notes breaks the rules")
{
    const auto broken = violations (Voicing {}, chordOf (0, "Major"), Difficulty::Advanced);

    REQUIRE (hasRule (broken, kRuleLowestNote));
    REQUIRE (hasRule (broken, kRuleChordTones));
    REQUIRE (hasRule (broken, kRuleStringCount));
}

TEST_CASE ("All shapes pass the filter at their own difficulty")
{
    const Fretboard fretboard;

    for (const auto& shape : bundledData ().shapes)
    {
        CAPTURE (shape.name);

        // Power chords and triads are too small for a strum, so they do not have that rule.
        const bool strummed = shape.chord != "Power" && shape.name.find ("Triad") == std::string::npos;

        int lowest = 0;
        for (const auto& fret : shape.frets)
            if (fret)
                lowest = std::min (lowest, *fret);

        const int firstRootFret = shape.open ? 0 : -lowest;
        const int lastRootFret  = shape.open ? 0 : 11;

        for (int rootFret = firstRootFret; rootFret <= lastRootFret; ++rootFret)
        {
            Voicing voicing;
            for (std::size_t s = 0; s < voicing.frets.size (); ++s)
                if (shape.frets[s])
                    voicing.frets[s] = *shape.frets[s] + rootFret;

            const auto rootIndex = shape.rootStringIndex ();
            const auto chord     = chordOf (pitchClassOf (fretboard.midiNoteAt (rootIndex, voicing.frets[static_cast<std::size_t> (rootIndex)])), shape.chord);

            CAPTURE (rootFret, textOf (voicing));
            REQUIRE (violations (voicing, chord, shape.difficulty, fretboard, strummed).empty ());
        }
    }
}

// The bad voicings of the defect test in Coding Spec Section 2 are not available. These bad voicings come
// from the old function, with each root on the low E string and the A string, and each chord type.
// The old function can also make a good voicing by accident, so this test finds each defect with its own code.
TEST_CASE ("The filter rejects each bad voicing from the old function")
{
    const Fretboard fretboard;

    int badVoicings = 0;

    for (int string = 0; string <= 1; ++string)
        for (int fret = 0; fret <= kMaxLegacyFret; ++fret)
            for (const auto& definition : bundledData ().chords.all ())
            {
                const auto positions = legacyTightVoicing (fretboard, { string, fret }, definition);
                const auto chord     = chordOf (pitchClassOf (fretboard.midiNoteAt (string, fret)), definition.name);

                std::set<int> strings;
                Voicing       voicing;
                MidiNote      lowest  = fretboard.midiNoteAt (string, fret);
                int           lowFret = 99;
                int           topFret = 0;

                for (const auto& position : positions)
                {
                    strings.insert (position.string);
                    voicing.frets[static_cast<std::size_t> (position.string)] = position.fret;
                    lowest                                                    = std::min (lowest, fretboard.midiNoteAt (position.string, position.fret));
                    if (position.fret > 0)
                    {
                        lowFret = std::min (lowFret, position.fret);
                        topFret = std::max (topFret, position.fret);
                    }
                }

                // A Voicing cannot hold two notes on a string, so the filter cannot see this defect.
                if (strings.size () != positions.size ())
                {
                    ++badVoicings;
                    continue;
                }

                const bool tooFewStrings = strings.size () < static_cast<std::size_t> (kMinStrummedStrings);
                const bool muteBetween   = *strings.rbegin () - *strings.begin () + 1 != static_cast<int> (strings.size ());
                const bool wrongLowest   = pitchClassOf (lowest) != chord.root;
                const bool tooWide       = topFret - lowFret > 4;

                if (!tooFewStrings && !muteBetween && !wrongLowest && !tooWide)
                    continue;

                CAPTURE (string, fret, definition.name, textOf (voicing));
                ++badVoicings;
                REQUIRE_FALSE (violations (voicing, chord, Difficulty::Intermediate, fretboard).empty ());
            }

    WARN ("Bad voicings from the old function: " << badVoicings);
    REQUIRE (badVoicings >= 55);
}

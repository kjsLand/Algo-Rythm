#include "algo/VoicingCost.h"

#include "support/VoicingHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

using namespace algo;
using algo::test::voicingOf;

TEST_CASE ("At Beginner, the open C voicing costs less than the C barre at fret 3")
{
    REQUIRE (staticCost (voicingOf ("x32010"), Difficulty::Beginner) < staticCost (voicingOf ("x35553"), Difficulty::Beginner));
}

TEST_CASE ("A barre and a high position cost more at Beginner")
{
    const auto barre = voicingOf ("133211");

    REQUIRE (staticCost (barre, Difficulty::Beginner) > staticCost (barre, Difficulty::Intermediate));
    REQUIRE (staticCost (barre, Difficulty::Intermediate) == staticCost (barre, Difficulty::Advanced));

    // With no barre and no fretted note, the cost is the same at all levels.
    REQUIRE (staticCost (voicingOf ("000000"), Difficulty::Beginner) == staticCost (voicingOf ("000000"), Difficulty::Advanced));

    // The same frets higher on the neck.
    REQUIRE (staticCost (voicingOf ("xx4322"), Difficulty::Beginner) > staticCost (voicingOf ("xx3211"), Difficulty::Beginner));
}

TEST_CASE ("At each difficulty, an open chord costs less than the same chord high on the neck")
{
    for (const auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
    {
        Voicing high;
        high.frets = { kMutedString, 0, 14, 14, 14, 0 };

        REQUIRE (staticCost (voicingOf ("x02220"), difficulty) < staticCost (high, difficulty));
    }
}

TEST_CASE ("Two fingers on one fret, more than 2 strings apart, have a cost")
{
    const CostWeights weights;

    CostWeights noReach;
    noReach.fingerReach = 0;

    const auto reach = [&] (const char* text)
    {
        return staticCost (voicingOf (text), Difficulty::Advanced) - staticCost (voicingOf (text), Difficulty::Advanced, Fretboard {}, noReach);
    };

    REQUIRE (reach ("x20402") == weights.fingerReach);
    REQUIRE (reach ("10321x") == weights.fingerReach);

    // The open A7: two fingers, 2 strings apart.
    REQUIRE (reach ("x02020") == 0);

    // A barre holds the notes with one finger.
    REQUIRE (reach ("133211") == 0);
}

TEST_CASE ("Two notes at the same fret have no barre cost if each note can have its own finger")
{
    CostWeights noBarre;
    noBarre.barreString = 0;

    REQUIRE (staticCost (voicingOf ("022000"), Difficulty::Beginner) == staticCost (voicingOf ("022000"), Difficulty::Beginner, Fretboard {}, noBarre));
    REQUIRE (staticCost (voicingOf ("133211"), Difficulty::Beginner) > staticCost (voicingOf ("133211"), Difficulty::Beginner, Fretboard {}, noBarre));
}

TEST_CASE ("Each weight changes the static cost")
{
    const auto cost = [] (const char* text)
    {
        return staticCost (voicingOf (text), Difficulty::Intermediate);
    };

    const CostWeights weights;

    // One more finger in the place of an open string. The mean fret stays at 2.
    REQUIRE (cost ("x32210") - cost ("x32010") == weights.finger + weights.openString);

    // Two more frets of span, with the same mean fret.
    REQUIRE (cost ("x4x01x") == cost ("x3x02x") + 2 * weights.span);

    // One more open string in the place of a muted string.
    REQUIRE (cost ("x32010") == cost ("x3201x") - weights.openString - weights.mutedString);

    // One more fret of hand position.
    REQUIRE (cost ("xxx222") - cost ("xxx111") == weights.position);
}

TEST_CASE ("The hand position is the mean fret of the fretted notes")
{
    REQUIRE (handPosition (voicingOf ("x32010")) == 2 * kHandUnitsPerFret);
    REQUIRE (handPosition (voicingOf ("000000")) == 0);
    REQUIRE (handPosition (Voicing {}) == 0);
    REQUIRE (handPosition (voicingOf ("xxxx12")) == 3 * kHandUnitsPerFret / 2);
}

TEST_CASE ("A lowest note out of the range E2 to A3 has a penalty")
{
    const CostWeights weights;

    // With a capo at fret 7, A2 becomes E3 and it stays in the range. A capo does not change the frets.
    const auto voicing = voicingOf ("x02220");
    REQUIRE (staticCost (voicing, Difficulty::Advanced) == staticCost (voicing, Difficulty::Advanced, Fretboard (Tuning {}, 7)));

    // With a capo at fret 3, the open G string (G3) becomes Bb3, above A3.
    REQUIRE (staticCost (voicingOf ("xxx000"), Difficulty::Advanced, Fretboard (Tuning {}, 3)) == staticCost (voicingOf ("xxx000"), Difficulty::Advanced) + weights.lowestNote);

    // A low tuning: D2.
    Tuning low;
    low.strings[0] = 38;
    REQUIRE (staticCost (voicingOf ("0xxxxx"), Difficulty::Advanced, Fretboard (low)) == staticCost (voicingOf ("0xxxxx"), Difficulty::Advanced) + weights.lowestNote);

    // A3 itself has no penalty, and neither has E2.
    Tuning a3;
    a3.strings[0] = kLowestNoteMax;
    REQUIRE (staticCost (voicingOf ("0xxxxx"), Difficulty::Advanced, Fretboard (a3)) == staticCost (voicingOf ("0xxxxx"), Difficulty::Advanced));
}

TEST_CASE ("Two voicings with 3 common tones have a lower transition cost than two voicings with 0 common tones")
{
    const auto c  = voicingOf ("x32010"); // C3 E3 G3 C4 E4
    const auto am = voicingOf ("x02210"); // A2 E3 A3 C4 E4: E3, C4 and E4 are common
    const auto d  = voicingOf ("xx0232"); // D3 A3 D4 F#4: no note is common

    // The two pairs have the same hand movement of 1/3 of a fret.
    REQUIRE (std::abs (handPosition (c) - handPosition (am)) == std::abs (handPosition (c) - handPosition (d)));

    const CostWeights weights;
    REQUIRE (transitionCost (c, am) < transitionCost (c, d));
    REQUIRE (transitionCost (c, d) - transitionCost (c, am) == 3 * weights.commonTone);
}

TEST_CASE ("A longer hand movement has a higher transition cost")
{
    const auto open = voicingOf ("x32010");

    REQUIRE (transitionCost (open, voicingOf ("133211")) < transitionCost (open, voicingOf ("577655")));

    const CostWeights weights;
    REQUIRE (transitionCost (voicingOf ("xxxx1x"), voicingOf ("xxxx3x")) == 2 * weights.handMove);
    REQUIRE (transitionCost (voicingOf ("xxxx3x"), voicingOf ("xxxx1x")) == 2 * weights.handMove);
}

TEST_CASE ("A note on two strings counts as one common tone")
{
    const CostWeights weights;

    // Each voicing has E4 on the high E string, and the B string at fret 5 is also E4.
    REQUIRE (transitionCost (voicingOf ("xxxx50"), voicingOf ("xxxxx0")) == 5 * weights.handMove - weights.commonTone);
}

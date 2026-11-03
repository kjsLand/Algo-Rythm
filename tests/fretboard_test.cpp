#include "algo/Fretboard.h"
#include "algo/Pitch.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <stdexcept>
#include <vector>

using namespace algo;

namespace
{
    const Tuning kDropD { "Drop D", { 38, 45, 50, 55, 59, 64 } };
    const Tuning kOpenG { "Open G", { 38, 43, 50, 55, 59, 62 } };
}

TEST_CASE ("The default fretboard has standard tuning and no capo")
{
    const Fretboard board;

    REQUIRE (board.capo () == 0);
    REQUIRE (board.numFrets () == 25);
    REQUIRE (board.midiNoteAt (0, 0) == 40);
    REQUIRE (board.midiNoteAt (5, 0) == 64);
    REQUIRE (board.midiNoteAt (5, 24) == 88);
    REQUIRE (board.noteNameAt (0, 0) == "E2");
}

TEST_CASE ("midiNoteAt adds the capo")
{
    const Fretboard board (Tuning {}, 2);

    REQUIRE (board.midiNoteAt (0, 0) == 42);
    REQUIRE (board.midiNoteAt (0, 3) == 45);
    REQUIRE (board.midiNoteAt (5, 0) == 66);
}

TEST_CASE ("midiNoteAt uses the tuning")
{
    const Fretboard dropD (kDropD);
    REQUIRE (dropD.midiNoteAt (0, 0) == 38);
    REQUIRE (dropD.midiNoteAt (1, 0) == 45);

    const Fretboard openG (kOpenG, 5);
    REQUIRE (openG.midiNoteAt (1, 0) == 48);
    REQUIRE (openG.tuning () == kOpenG);
    REQUIRE (openG.capo () == 5);
}

TEST_CASE ("noteNameAt uses the spelling of the key")
{
    const Fretboard board;

    REQUIRE (board.noteNameAt (0, 6) == "A#2");
    REQUIRE (board.noteNameAt (0, 6, Key { 5, "Major" }) == "Bb2");
}

TEST_CASE ("The capo must be 0 to 7")
{
    for (int capo = 0; capo <= 7; ++capo)
        REQUIRE (Fretboard (Tuning {}, capo).capo () == capo);

    REQUIRE_THROWS_AS (Fretboard (Tuning {}, -1), std::invalid_argument);
    REQUIRE_THROWS_AS (Fretboard (Tuning {}, 8), std::invalid_argument);
}

TEST_CASE ("A position that is not on the neck throws std::out_of_range")
{
    const Fretboard board (Tuning {}, 2);

    REQUIRE_THROWS_AS (board.midiNoteAt (-1, 0), std::out_of_range);
    REQUIRE_THROWS_AS (board.midiNoteAt (6, 0), std::out_of_range);
    REQUIRE_THROWS_AS (board.midiNoteAt (0, -1), std::out_of_range);
    REQUIRE (board.midiNoteAt (0, 22) == 64);
    REQUIRE_THROWS_AS (board.midiNoteAt (0, 23), std::out_of_range);
    REQUIRE_THROWS_AS (board.noteNameAt (0, 23), std::out_of_range);
}

TEST_CASE ("findNote returns only positions at or above the capo")
{
    // String 0 is 40 with no capo, so MIDI note 41 is at fret 1.
    REQUIRE (Fretboard ().findNote (41) == std::vector<FretPos> { { 0, 1 } });

    const Fretboard board (Tuning {}, 2);
    REQUIRE (board.findNote (40).empty ());
    REQUIRE (board.findNote (41).empty ());
    REQUIRE (board.findNote (42) == std::vector<FretPos> { { 0, 0 } });
}

TEST_CASE ("findNote returns no position above the top of the neck")
{
    REQUIRE_FALSE (Fretboard ().findNote (88).empty ());
    REQUIRE (Fretboard ().findNote (89).empty ());

    // The capo does not change the highest note.
    REQUIRE (Fretboard (Tuning {}, 7).findNote (88) == std::vector<FretPos> { { 5, 17 } });
    REQUIRE (Fretboard (Tuning {}, 7).findNote (89).empty ());
}

TEST_CASE ("Each position from findNote plays the same note")
{
    for (const auto& tuning : { Tuning {}, kDropD, kOpenG })
        for (int capo = 0; capo <= Fretboard::kMaxCapo; ++capo)
        {
            const Fretboard board (tuning, capo);

            for (MidiNote midi = 0; midi <= 127; ++midi)
            {
                CAPTURE (tuning.name, capo, midi);

                for (const auto& pos : board.findNote (midi))
                    REQUIRE (board.midiNoteAt (pos.string, pos.fret) == midi);
            }
        }
}

TEST_CASE ("findNote finds each position on the neck")
{
    const Fretboard board (kDropD, 3);

    int count = 0;
    board.forEach ([&board, &count] (const FretPos& pos, MidiNote midi)
    {
        ++count;

        const auto found = board.findNote (midi);
        REQUIRE (std::find (found.begin (), found.end (), pos) != found.end ());
    });

    REQUIRE (count == kNumStrings * board.numFrets ());
}

TEST_CASE ("findNoteByName uses the capo")
{
    const Fretboard board (Tuning {}, 2);

    REQUIRE (board.findNoteByName ("F#2") == std::vector<FretPos> { { 0, 0 } });
    REQUIRE (board.findNoteByName ("E2").empty ());

    const auto all = board.findNoteByName ("E");
    REQUIRE_FALSE (all.empty ());
    for (const auto& pos : all)
        REQUIRE (pitchClassOf (board.midiNoteAt (pos.string, pos.fret)) == 4);
}

TEST_CASE ("scalePositions returns only notes in the scale")
{
    const Fretboard board (kDropD, 1);
    const Key       key { 7, "Major Pentatonic" };

    const auto positions = board.scalePositions (key);
    REQUIRE_FALSE (positions.empty ());

    std::size_t expected = 0;
    board.forEach ([&expected] (const FretPos&, MidiNote midi)
    {
        const PitchClass pc = pitchClassOf (midi);
        if (pc == 7 || pc == 9 || pc == 11 || pc == 2 || pc == 4)
            ++expected;
    });
    REQUIRE (positions.size () == expected);

    REQUIRE_THROWS_AS (board.scalePositions (Key { 0, "Klingon" }), std::out_of_range);
}

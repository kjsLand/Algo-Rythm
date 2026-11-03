#include "algo/Data.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

using namespace algo;

namespace
{
    const Tuning kStandard;

    const std::size_t kNumStringsSize = static_cast<std::size_t> (kNumStrings);

    int lowestRelativeFret (const ChordShape& shape)
    {
        int lowest = 0;
        for (const auto& fret : shape.frets)
            if (fret)
                lowest = std::min (lowest, *fret);
        return lowest;
    }

    // Returns the MIDI notes that sound, from the low E string to the high E string.
    std::vector<MidiNote> play (const ChordShape& shape, int rootFret)
    {
        std::vector<MidiNote> notes;
        for (std::size_t i = 0; i < kNumStringsSize; ++i)
            if (shape.frets[i])
                notes.push_back (kStandard.strings[i] + rootFret + *shape.frets[i]);
        return notes;
    }

    // Checks that the notes are the chord tones of the root. The optional tones can be absent.
    void requireChordTones (const ChordShape& shape, const ChordDefinition& chord, int rootFret)
    {
        const int rootNote = kStandard.strings[static_cast<std::size_t> (shape.rootStringIndex ())] + rootFret + *shape.frets[static_cast<std::size_t> (shape.rootStringIndex ())];

        CAPTURE (rootFret);
        const auto notes = play (shape, rootFret);

        // Each root-position shape has the root as its lowest note (REQ-07-2).
        REQUIRE (*std::min_element (notes.begin (), notes.end ()) == rootNote);

        std::set<PitchClass> sounding;
        for (MidiNote note : notes)
            sounding.insert (note % 12);

        const auto           chordTones = ChordRegistry::pitchClasses (rootNote % 12, chord);
        std::set<PitchClass> allowed (chordTones.begin (), chordTones.end ());
        std::set<PitchClass> required (allowed);
        for (int interval : chord.optional)
            required.erase ((rootNote + interval) % 12);

        for (PitchClass pc : sounding)
        {
            CAPTURE (pc);
            REQUIRE (allowed.count (pc) == 1);
        }
        for (PitchClass pc : required)
        {
            CAPTURE (pc);
            REQUIRE (sounding.count (pc) == 1);
        }
    }
}

TEST_CASE ("The file contains 40 or more shapes and each chord type has a shape")
{
    const auto data = loadBundledData ();

    REQUIRE (data.shapes.size () >= 40);

    for (const auto& chord : data.chords.all ())
    {
        CAPTURE (chord.name);
        REQUIRE (std::any_of (data.shapes.begin (), data.shapes.end (), [&chord] (const ChordShape& shape)
        {
            return shape.chord == chord.name;
        }));
    }
}

TEST_CASE ("The shapes that the spec names are present")
{
    const auto data = loadBundledData ();

    std::set<std::string> names;
    for (const auto& shape : data.shapes)
        names.insert (shape.name);

    for (const char* name : { "Open C", "Open A", "Open G", "Open E", "Open D", "Open Am", "Open Em", "Open Dm",
                              "Open A7", "Open D7", "Open E7", "Open G7", "Open B7", "Open Cadd9", "Open Dsus2", "Open Dsus4",
                              "E-Shape Major", "E-Shape Minor", "E-Shape Dom7", "E-Shape M7",
                              "A-Shape Major", "A-Shape Minor", "A-Shape Dom7", "A-Shape M7",
                              "Power Root 6", "Power Root 5",
                              "Major Triad Strings 1-3", "Minor Triad Strings 1-3",
                              "Major Triad Strings 2-4", "Minor Triad Strings 2-4",
                              "Major Triad Strings 3-5", "Minor Triad Strings 3-5" })
    {
        CAPTURE (name);
        REQUIRE (names.count (name) == 1);
    }
}

TEST_CASE ("Each shape played at its root fret gives the correct pitch classes")
{
    const auto data = loadBundledData ();

    for (const auto& shape : data.shapes)
    {
        CAPTURE (shape.name);
        const auto& chord = data.chords.get (shape.chord);

        if (shape.open)
        {
            requireChordTones (shape, chord, 0);
            continue;
        }

        // A movable shape must work on each root fret that keeps it on the neck.
        for (int rootFret = -lowestRelativeFret (shape); rootFret <= 12; ++rootFret)
            requireChordTones (shape, chord, rootFret);
    }
}

TEST_CASE ("Open shapes have the correct root")
{
    const auto data = loadBundledData ();

    const auto rootOf = [&data] (const std::string& name)
    {
        const auto shape = std::find_if (data.shapes.begin (), data.shapes.end (), [&name] (const ChordShape& s)
        {
            return s.name == name;
        });
        REQUIRE (shape != data.shapes.end ());
        const auto index = static_cast<std::size_t> (shape->rootStringIndex ());
        return (kStandard.strings[index] + *shape->frets[index]) % 12;
    };

    REQUIRE (rootOf ("Open C") == 0);
    REQUIRE (rootOf ("Open G") == 7);
    REQUIRE (rootOf ("Open Dm") == 2);
    REQUIRE (rootOf ("Open B7") == 11);
    REQUIRE (rootOf ("Open Fmaj7") == 5);
}

TEST_CASE ("Each shape can be played")
{
    for (const auto& shape : loadBundledData ().shapes)
    {
        CAPTURE (shape.name);

        std::set<int> usedFingers;
        int           sharedFingers = 0;
        int           lowest        = 99;
        int           highest       = -99;
        int           firstSound    = -1;
        int           lastSound     = -1;
        int           soundCount    = 0;

        for (std::size_t i = 0; i < kNumStringsSize; ++i)
        {
            CAPTURE (i);
            const auto& fret   = shape.frets[i];
            const int   finger = shape.fingers[i];

            if (fret)
            {
                ++soundCount;
                lastSound = static_cast<int> (i);
                if (firstSound < 0)
                    firstSound = lastSound;
            }

            // A finger is on a string only if the string has a fretted note.
            const bool fretted = fret && (!shape.open || *fret > 0);
            REQUIRE (fretted == (finger > 0));

            if (fretted)
            {
                lowest  = std::min (lowest, *fret);
                highest = std::max (highest, *fret);
                if (!usedFingers.insert (finger).second)
                    ++sharedFingers;
            }
        }

        // REQ-07-4: the fret span is 3 frets or fewer at Beginner, and 4 frets or fewer at other levels.
        REQUIRE (highest - lowest <= (shape.difficulty == Difficulty::Beginner ? 3 : 4));

        // A barre is the only way that one finger can cover more than one string.
        REQUIRE (shape.barre == (sharedFingers > 0));

        // REQ-07-7: no muted string between two strings that sound.
        REQUIRE (lastSound - firstSound + 1 == soundCount);

        // Shapes for strumming have 4 or more strings. Power chords and triads have fewer.
        if (shape.chord != "Power" && shape.name.find ("Triad") == std::string::npos)
            REQUIRE (soundCount >= 4);
    }
}

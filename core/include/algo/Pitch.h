#pragma once

#include "Types.h"

#include <optional>
#include <string>

namespace algo
{
    // Returns the pitch class of a note. The result is 0-11, also for a negative value.
    inline PitchClass pitchClassOf (int note)
    {
        return ((note % 12) + 12) % 12;
    }

    struct ParsedNote
    {
        PitchClass              pitchClass { 0 };
        std::optional<MidiNote> midiNote; // No value if the name has no octave number
    };

    // Accepts a letter, accidentals and an optional octave number: "C", "c", "Db", "B#", "F#3", "C-1".
    // Accidentals keep the written octave, so "B#3" is MIDI note 60 and "Cb4" is MIDI note 59.
    // Returns no value if the name is not valid or its note is not in the MIDI range 0-127.
    std::optional<ParsedNote> tryParseNoteName (const std::string& name);

    // The octave number is optional. Throws std::invalid_argument if the name is not valid.
    PitchClass parsePitchClass (const std::string& name);

    // The octave number is necessary. "C4" is 60. Throws std::invalid_argument if the name is not valid.
    MidiNote parseMidiNote (const std::string& name);

    // True for F, Bb, Eb, Ab, Db and Gb major and for each scale with the same notes,
    // for example D Natural Minor and G Dorian. A scale with no major parent, for example
    // Whole Tone, follows the major scale on its root.
    bool usesFlats (const Key& key);

    // Spells a pitch class with sharps or with flats, with no key.
    std::string pitchClassName (PitchClass pc, bool flats);

    // Uses flats if usesFlats (key) is true. Gb major has the note Cb, so its pitch class 11 is "Cb".
    std::string pitchClassName (PitchClass pc, const Key& key);

    // Adds the octave number. MIDI note 60 is C4. The result parses back to the same note.
    std::string midiNoteName (MidiNote note, const Key& key);
}

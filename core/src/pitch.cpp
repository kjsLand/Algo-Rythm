#include "algo/Pitch.h"

#include "data_reader.h"

#include <array>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace algo
{
    namespace
    {
        // The semitones from the key root up to the major scale that has the same notes.
        int semitonesToParentMajor (const std::string& scale)
        {
            static const std::unordered_map<std::string, int> offsets {
                { "natural minor", 3 },
                { "harmonic minor", 3 },
                { "melodic minor", 3 },
                { "minor pentatonic", 3 },
                { "blues", 3 },
                { "dorian", 10 },
                { "phrygian", 8 },
                { "lydian", 7 },
                { "mixolydian", 5 },
                { "locrian", 1 },
            };

            const auto it = offsets.find (detail::toLower (scale));
            return it == offsets.end () ? 0 : it->second;
        }

        PitchClass parentMajorRoot (const Key& key)
        {
            return pitchClassOf (key.root + semitonesToParentMajor (key.scale));
        }
    }

    std::optional<ParsedNote> tryParseNoteName (const std::string& name)
    {
        static constexpr std::array<int, 7> naturals { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G

        if (name.empty ())
            return std::nullopt;

        const int letter = std::toupper (static_cast<unsigned char> (name[0]));
        if (letter < 'A' || letter > 'G')
            return std::nullopt;

        int         note = naturals[static_cast<std::size_t> (letter - 'A')];
        std::size_t pos  = 1;
        for (; pos < name.size () && (name[pos] == '#' || name[pos] == 'b'); ++pos)
            note += name[pos] == '#' ? 1 : -1;

        ParsedNote parsed;
        parsed.pitchClass = pitchClassOf (note);
        if (pos == name.size ())
            return parsed;

        // The octave number has an optional minus sign and 1 or 2 digits.
        const bool negative = name[pos] == '-';
        if (negative)
            ++pos;

        const std::size_t digits = name.size () - pos;
        if (digits < 1 || digits > 2)
            return std::nullopt;

        int octave = 0;
        for (; pos < name.size (); ++pos)
        {
            if (!std::isdigit (static_cast<unsigned char> (name[pos])))
                return std::nullopt;
            octave = octave * 10 + (name[pos] - '0');
        }

        const MidiNote midi = ((negative ? -octave : octave) + 1) * 12 + note;
        if (midi < 0 || midi > 127)
            return std::nullopt;

        parsed.midiNote = midi;
        return parsed;
    }

    PitchClass parsePitchClass (const std::string& name)
    {
        const auto parsed = tryParseNoteName (name);
        if (!parsed)
            throw std::invalid_argument ("The note name is not valid: '" + name + "'");
        return parsed->pitchClass;
    }

    MidiNote parseMidiNote (const std::string& name)
    {
        const auto parsed = tryParseNoteName (name);
        if (!parsed || !parsed->midiNote)
            throw std::invalid_argument ("The MIDI note name is not valid: '" + name + "'");
        return *parsed->midiNote;
    }

    bool usesFlats (const Key& key)
    {
        switch (parentMajorRoot (key))
        {
            case 1:  // Db
            case 3:  // Eb
            case 5:  // F
            case 6:  // Gb
            case 8:  // Ab
            case 10: // Bb
                return true;
            default:
                return false;
        }
    }

    std::string pitchClassName (PitchClass pc, bool flats)
    {
        static const std::array<std::string, 12> sharpNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        static const std::array<std::string, 12> flatNames { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };

        return (flats ? flatNames : sharpNames)[static_cast<std::size_t> (pitchClassOf (pc))];
    }

    std::string pitchClassName (PitchClass pc, const Key& key)
    {
        if (pitchClassOf (pc) == 11 && parentMajorRoot (key) == 6)
            return "Cb";
        return pitchClassName (pc, usesFlats (key));
    }

    std::string midiNoteName (MidiNote note, const Key& key)
    {
        const std::string name = pitchClassName (note, key);

        // Cb is one semitone below the C of its written octave.
        const int octave = (note - pitchClassOf (note)) / 12 - 1 + (name == "Cb" ? 1 : 0);
        return name + std::to_string (octave);
    }
}

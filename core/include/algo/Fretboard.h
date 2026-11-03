#pragma once

#include "Data.h"
#include "Types.h"

#include <functional>
#include <string>
#include <vector>

namespace algo
{
    // The neck of a 6-string guitar with a tuning and a capo.
    //
    // Frets count from the capo: fret 0 is the note at the capo and it plays as an open string.
    // A position cannot be below the capo, and the capo removes frets from the top of the range.
    class Fretboard
    {
    public:
        static constexpr int kNeckFrets = 25; // Frets 0-24 with no capo
        static constexpr int kMaxCapo   = 7;

        // Standard tuning with no capo.
        Fretboard () = default;

        // Throws std::invalid_argument if the capo is not in the range 0 to 7.
        explicit Fretboard (const Tuning& tuning, int capo = 0);

        const Tuning& tuning () const
        {
            return m_tuning;
        }

        int capo () const
        {
            return m_capo;
        }

        // The number of frets above the capo, with fret 0.
        int numFrets () const
        {
            return kNeckFrets - m_capo;
        }

        // String 0 is the lowest string in standard tuning.
        // These functions throw std::out_of_range if the position is not on the neck.
        MidiNote    midiNoteAt (int string, int fret) const;
        std::string noteNameAt (int string, int fret, const Key& key = Key {}) const;

        std::vector<FretPos> findNote (MidiNote midi) const;

        // A name with an octave number finds one note. A name with no octave number finds
        // its pitch class on all of the neck. A name that is not valid finds nothing.
        std::vector<FretPos> findNoteByName (const std::string& name) const;

        // Finds each position with a pitch class in the scale. Throws std::out_of_range if the scale is unknown.
        std::vector<FretPos> scalePositions (const Key& key, const DataSet& data = bundledData ()) const;

        // Calls fn for each position, from the lowest fret of string 0 to the highest fret of string 5.
        void forEach (const std::function<void (const FretPos&, MidiNote)>& fn) const;

    private:
        Tuning m_tuning;
        int    m_capo { 0 };
    };
}

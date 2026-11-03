#include "algo/Fretboard.h"

#include "algo/Pitch.h"
#include "algo/Scale.h"

#include <algorithm>
#include <stdexcept>

namespace algo
{
    Fretboard::Fretboard (const Tuning& tuning, int capo)
        : m_tuning (tuning),
          m_capo (capo)
    {
        if (capo < 0 || capo > kMaxCapo)
            throw std::invalid_argument ("The capo must be 0 to 7, not " + std::to_string (capo));
    }

    MidiNote Fretboard::midiNoteAt (int string, int fret) const
    {
        if (string < 0 || string >= kNumStrings || fret < 0 || fret >= numFrets ())
            throw std::out_of_range ("The position is not on the neck: string " + std::to_string (string) + ", fret " + std::to_string (fret));

        return m_tuning.strings[static_cast<std::size_t> (string)] + m_capo + fret;
    }

    std::string Fretboard::noteNameAt (int string, int fret, const Key& key) const
    {
        return midiNoteName (midiNoteAt (string, fret), key);
    }

    std::vector<FretPos> Fretboard::findNote (MidiNote midi) const
    {
        std::vector<FretPos> results;
        forEach ([&results, midi] (const FretPos& pos, MidiNote note)
        {
            if (note == midi)
                results.push_back (pos);
        });
        return results;
    }

    std::vector<FretPos> Fretboard::findNoteByName (const std::string& name) const
    {
        const auto parsed = tryParseNoteName (name);
        if (!parsed)
            return {};

        if (parsed->midiNote)
            return findNote (*parsed->midiNote);

        std::vector<FretPos> results;
        forEach ([&results, pc = parsed->pitchClass] (const FretPos& pos, MidiNote note)
        {
            if (pitchClassOf (note) == pc)
                results.push_back (pos);
        });
        return results;
    }

    std::vector<FretPos> Fretboard::scalePositions (const Key& key, const DataSet& data) const
    {
        const auto scale = scalePitchClasses (key, data);

        std::vector<FretPos> results;
        forEach ([&results, &scale] (const FretPos& pos, MidiNote note)
        {
            if (std::find (scale.begin (), scale.end (), pitchClassOf (note)) != scale.end ())
                results.push_back (pos);
        });
        return results;
    }

    void Fretboard::forEach (const std::function<void (const FretPos&, MidiNote)>& fn) const
    {
        for (int s = 0; s < kNumStrings; ++s)
            for (int f = 0; f < numFrets (); ++f)
                fn ({ s, f }, midiNoteAt (s, f));
    }
}

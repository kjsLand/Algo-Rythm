#pragma once

#include "algo/Data.h"
#include "algo/Voicing.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace algo::test
{
    /** Parses text such as "x32010": one digit for each string from the low E string to the high E string. */
    inline Voicing voicingOf (const std::string& text)
    {
        Voicing voicing;
        for (std::size_t s = 0; s < text.size (); ++s)
            if (text[s] != 'x')
                voicing.frets[s] = text[s] - '0';
        return voicing;
    }

    /** Returns text such as "x32010". A fret above 9 goes in brackets, as in "[10]". */
    inline std::string textOf (const Voicing& voicing)
    {
        std::string text;
        for (int fret : voicing.frets)
        {
            if (fret == kMutedString)
                text += 'x';
            else if (fret < 10)
                text += std::to_string (fret);
            else
                text += "[" + std::to_string (fret) + "]";
        }
        return text;
    }

    inline ConcreteChord chordOf (PitchClass root, const std::string& type, std::optional<PitchClass> bass = std::nullopt)
    {
        return { root, bundledData ().chords.get (type), bass, std::to_string (root) + " " + type };
    }

    inline bool hasVoicing (const std::vector<Voicing>& voicings, const std::string& text)
    {
        return std::any_of (voicings.begin (), voicings.end (), [&text] (const Voicing& voicing)
        {
            return textOf (voicing) == text;
        });
    }

    inline bool hasRule (const std::vector<std::string>& rules, const std::string& rule)
    {
        return std::find (rules.begin (), rules.end (), rule) != rules.end ();
    }
}

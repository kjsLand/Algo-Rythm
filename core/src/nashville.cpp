#include "algo/Nashville.h"

#include "algo/Diatonic.h"
#include "algo/Pitch.h"

#include <regex>
#include <stdexcept>

namespace algo
{
    namespace
    {
        int parseAccidental (const std::string& text)
        {
            return text == "b" ? -1 : (text == "#" ? 1 : 0);
        }

        std::string accidentalText (int accidental)
        {
            return accidental < 0 ? "b" : (accidental > 0 ? "#" : "");
        }

        std::string degreeName (int degree, int accidental, const Key& key, const DataSet& data)
        {
            const PitchClass pc = pitchClassOf (degreePitchClass (key, degree, data) + accidental);
            return accidental == 0 ? pitchClassName (pc, key) : pitchClassName (pc, accidental < 0);
        }
    }

    ChordSymbol parseNumeral (const std::string& text)
    {
        static const std::regex format ("([b#]?)([1-7])([^/]*)(?:/([b#]?)([1-7]))?");

        std::smatch match;
        if (!std::regex_match (text, match, format) || match[3] == kDiatonicQuality)
            throw std::invalid_argument ("The numeral is not valid: '" + text + "'");

        ChordSymbol symbol;
        symbol.accidental = parseAccidental (match[1]);
        symbol.degree     = std::stoi (match[2]);
        if (match[3].length () > 0)
            symbol.quality = match[3];
        if (match[5].matched)
        {
            symbol.bassAccidental = parseAccidental (match[4]);
            symbol.bassDegree     = std::stoi (match[5]);
        }
        return symbol;
    }

    ChordSymbol resolve (const ChordSymbol& symbol, const Key& key, const DataSet& data)
    {
        ChordSymbol resolved = symbol;
        if (symbol.quality != kDiatonicQuality)
        {
            if (!data.chords.containsSymbol (symbol.quality))
                throw std::invalid_argument ("The chord symbol is not in the registry: '" + symbol.quality + "'");
        }
        else if (symbol.accidental != 0)
            resolved.quality = data.chords.get ("Major").symbol;
        else
            resolved.quality = diatonicTriad (key, symbol.degree, data).chord.symbol;

        return resolved;
    }

    PitchClass chordRootPc (const ChordSymbol& symbol, const Key& key, const DataSet& data)
    {
        return pitchClassOf (degreePitchClass (key, symbol.degree, data) + symbol.accidental);
    }

    std::string chordName (const ChordSymbol& symbol, const Key& key, const DataSet& data)
    {
        std::string name = degreeName (symbol.degree, symbol.accidental, key, data) + resolve (symbol, key, data).quality;
        if (symbol.bassDegree != 0)
            name += "/" + degreeName (symbol.bassDegree, symbol.bassAccidental, key, data);
        return name;
    }

    std::string numeralText (const ChordSymbol& symbol)
    {
        std::string text = accidentalText (symbol.accidental) + std::to_string (symbol.degree);
        if (symbol.quality != kDiatonicQuality)
            text += symbol.quality;
        if (symbol.bassDegree != 0)
            text += "/" + accidentalText (symbol.bassAccidental) + std::to_string (symbol.bassDegree);
        return text;
    }
}

#pragma once

#include "Rng.h"

#include <array>
#include <optional>
#include <string>

namespace algo
{
    constexpr int kDiatonicNumerals = 7;

    // The table for a scale with the tag "minor" is Minor. The table for all other scales is Major.
    enum class TransitionMode
    {
        Major,
        Minor
    };

    // allowed[d - 1] is true if the walk can use the numeral d.
    using NumeralMask = std::array<bool, kDiatonicNumerals>;

    // The weights of the numerals that can follow each diatonic numeral.
    // A numeral is a scale degree from 1 to 7, so it has the chord of the scale in use.
    class TransitionTable
    {
    public:
        // Throws DataError if a table or a row is missing, or if a row does not add up to 1.0 within 0.001.
        static TransitionTable fromJsonText (const std::string& text, const std::string& file = "transitions.json");

        // Tells where the weights come from, for example "hand-authored v1".
        const std::string& source () const
        {
            return m_source;
        }

        // Throws std::out_of_range if a numeral is not 1 to 7.
        double weight (TransitionMode mode, int from, int to) const;

        // Returns the next numeral with weightedIndex. The result is an allowed numeral and is never `from`.
        // Returns no value if no such numeral has a weight.
        std::optional<int> next (TransitionMode mode, int from, const NumeralMask& allowed, Rng& rng) const;

    private:
        using Row = std::array<double, kDiatonicNumerals>;

        std::array<std::array<Row, kDiatonicNumerals>, 2> m_weights {};
        std::string                                       m_source;
    };
}

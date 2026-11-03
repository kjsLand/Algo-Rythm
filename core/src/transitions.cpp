#include "algo/Transitions.h"

#include "algo/DataError.h"

#include "data_reader.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace algo
{
    namespace
    {
        constexpr double kWeightTolerance = 0.001;

        std::optional<int> parseDiatonicNumeral (const std::string& text)
        {
            if (text.size () != 1 || text[0] < '1' || text[0] > '0' + kDiatonicNumerals)
                return std::nullopt;
            return text[0] - '0';
        }

        std::size_t tableIndex (TransitionMode mode)
        {
            return mode == TransitionMode::Major ? 0 : 1;
        }

        std::size_t numeralIndex (int numeral)
        {
            return static_cast<std::size_t> (numeral - 1);
        }
    }

    TransitionTable TransitionTable::fromJsonText (const std::string& text, const std::string& file)
    {
        TransitionTable     table;
        std::array<bool, 2> found { false, false };

        for (const auto& entry : detail::readEntries (text, file, "tables"))
        {
            if (entry.name () != "major" && entry.name () != "minor")
                entry.fail ("the table must be 'major' or 'minor'");

            const auto mode          = entry.name () == "major" ? TransitionMode::Major : TransitionMode::Minor;
            found[tableIndex (mode)] = true;

            const Json& rows = entry.getObject ("rows");
            for (int numeral = 1; numeral <= kDiatonicNumerals; ++numeral)
                if (!rows.contains (std::to_string (numeral)))
                    entry.fail ("the numeral '" + std::to_string (numeral) + "' has no row");

            for (const auto& row : rows.items ())
            {
                const auto from = parseDiatonicNumeral (row.key ());
                if (!from)
                    entry.fail ("the row '" + row.key () + "' is not a diatonic numeral");
                if (!row.value ().is_object ())
                    entry.fail ("the row '" + row.key () + "' must be an object");

                double sum = 0.0;
                for (const auto& cell : row.value ().items ())
                {
                    const auto to = parseDiatonicNumeral (cell.key ());
                    if (!to)
                        entry.fail ("the row '" + row.key () + "' has '" + cell.key () + "', which is not a diatonic numeral");
                    if (!cell.value ().is_number () || cell.value ().get<double> () < 0.0)
                        entry.fail ("the row '" + row.key () + "' must have weights of 0 or more");

                    const auto weight = cell.value ().get<double> ();
                    sum += weight;
                    table.m_weights[tableIndex (mode)][numeralIndex (*from)][numeralIndex (*to)] = weight;
                }

                if (std::abs (sum - 1.0) > kWeightTolerance)
                    entry.fail ("the row '" + row.key () + "' adds up to " + std::to_string (sum) + ", not 1.0");
            }
        }

        if (!found[tableIndex (TransitionMode::Major)])
            throw DataError (file, "", "the table 'major' is missing");
        if (!found[tableIndex (TransitionMode::Minor)])
            throw DataError (file, "", "the table 'minor' is missing");

        // readEntries has accepted the text, so it parses.
        const Json document = Json::parse (text);
        if (!document.contains ("source"))
            throw DataError (file, "", "the required field 'source' is missing");
        if (!document.at ("source").is_string ())
            throw DataError (file, "", "the field 'source' must be a string");
        table.m_source = document.at ("source").get<std::string> ();

        return table;
    }

    double TransitionTable::weight (TransitionMode mode, int from, int to) const
    {
        return m_weights[tableIndex (mode)].at (numeralIndex (from)).at (numeralIndex (to));
    }

    std::optional<int> TransitionTable::next (TransitionMode mode, int from, const NumeralMask& allowed, Rng& rng) const
    {
        std::vector<double> weights;
        for (int to = 1; to <= kDiatonicNumerals; ++to)
            weights.push_back (to != from && allowed[numeralIndex (to)] ? weight (mode, from, to) : 0.0);

        const auto hasWeight = [] (double value)
        {
            return value > 0.0;
        };
        if (std::none_of (weights.begin (), weights.end (), hasWeight))
            return std::nullopt;

        return static_cast<int> (rng.weightedIndex (weights)) + 1;
    }
}

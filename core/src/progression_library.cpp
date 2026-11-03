#include "algo/ProgressionLibrary.h"

#include "algo/Nashville.h"

#include "data_reader.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace algo
{
    namespace
    {
        bool hasItem (const std::vector<std::string>& items, const std::string& item)
        {
            return std::find (items.begin (), items.end (), item) != items.end ();
        }

        void checkNumeral (const detail::EntryReader& entry, const std::string& numeral, const ChordRegistry& chords)
        {
            ChordSymbol symbol;
            try
            {
                symbol = parseNumeral (numeral);
            }
            catch (const std::invalid_argument&)
            {
                entry.fail ("the numeral '" + numeral + "' is not valid");
            }

            if (symbol.quality != kDiatonicQuality && !chords.containsSymbol (symbol.quality))
                entry.fail ("the numeral '" + numeral + "' has the unknown chord symbol '" + symbol.quality + "'");
        }
    }

    ProgressionLibrary ProgressionLibrary::fromJsonText (const std::string&   text,
                                                         const ScaleRegistry& scales,
                                                         const ChordRegistry& chords,
                                                         const std::string&   file)
    {
        static const std::vector<std::string> validRoles { "verse", "chorus", "bridge", "loop" };

        ProgressionLibrary library;
        for (const auto& entry : detail::readEntries (text, file, "progressions"))
        {
            ProgressionDefinition def { entry.name (),
                                        entry.getStringList ("numerals"),
                                        entry.getString ("mode"),
                                        entry.getStringList ("tags"),
                                        entry.getStringList ("roles"),
                                        entry.getDifficulty ("difficulty") };

            if (def.numerals.empty ())
                entry.fail ("the field 'numerals' must not be empty");
            for (const auto& numeral : def.numerals)
                checkNumeral (entry, numeral, chords);

            if (!scales.contains (def.mode))
                entry.fail ("the mode '" + def.mode + "' is not in the scale registry");

            if (def.roles.empty ())
                entry.fail ("the field 'roles' must not be empty");
            for (const auto& role : def.roles)
                if (!hasItem (validRoles, role))
                    entry.fail ("the role '" + role + "' is not valid");

            library.m_progressions.push_back (std::move (def));
        }
        return library;
    }

    std::vector<ProgressionDefinition> ProgressionLibrary::filter (const ProgressionFilter& filter) const
    {
        std::vector<ProgressionDefinition> out;
        for (const auto& def : m_progressions)
        {
            if (!filter.mode.empty () && detail::toLower (filter.mode) != detail::toLower (def.mode))
                continue;
            if (!filter.role.empty () && !hasItem (def.roles, filter.role))
                continue;
            if (!filter.tag.empty () && !hasItem (def.tags, filter.tag))
                continue;
            if (filter.difficulty && def.difficulty > *filter.difficulty)
                continue;
            out.push_back (def);
        }
        return out;
    }
}

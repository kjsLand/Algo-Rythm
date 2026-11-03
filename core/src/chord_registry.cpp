#include "algo/ChordRegistry.h"

#include "data_reader.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace algo
{
    ChordRegistry ChordRegistry::fromJsonText (const std::string& text, const std::string& file)
    {
        ChordRegistry registry;
        for (const auto& entry : detail::readEntries (text, file, "chords"))
        {
            ChordDefinition def { entry.name (),
                                  entry.getString ("symbol"),
                                  entry.getIntList ("intervals"),
                                  entry.getIntList ("optional"),
                                  entry.getStringList ("tags") };

            if (def.intervals.empty ())
                entry.fail ("the field 'intervals' must not be empty");

            for (int interval : def.intervals)
                if (interval <= 0)
                    entry.fail ("each interval must be larger than 0");

            for (int interval : def.optional)
                if (std::find (def.intervals.begin (), def.intervals.end (), interval) == def.intervals.end ())
                    entry.fail ("the optional interval " + std::to_string (interval) + " is not in 'intervals'");

            if (!registry.m_symbols.emplace (def.symbol, registry.m_chords.size ()).second)
                entry.fail ("the symbol '" + def.symbol + "' is not unique in the file");

            registry.m_index[detail::toLower (def.name)] = registry.m_chords.size ();
            registry.m_chords.push_back (std::move (def));
        }
        return registry;
    }

    const ChordDefinition& ChordRegistry::get (const std::string& name) const
    {
        const auto it = m_index.find (detail::toLower (name));
        if (it == m_index.end ())
            throw std::out_of_range ("Unknown chord: " + name);
        return m_chords[it->second];
    }

    bool ChordRegistry::contains (const std::string& name) const
    {
        return m_index.count (detail::toLower (name)) > 0;
    }

    bool ChordRegistry::containsSymbol (const std::string& symbol) const
    {
        return m_symbols.count (symbol) > 0;
    }

    std::vector<std::string> ChordRegistry::allNames () const
    {
        std::vector<std::string> names;
        names.reserve (m_chords.size ());
        for (const auto& def : m_chords)
            names.push_back (def.name);
        return names;
    }

    std::vector<PitchClass> ChordRegistry::pitchClasses (PitchClass root, const ChordDefinition& def)
    {
        std::vector<PitchClass> pcs { root % 12 };
        for (int interval : def.intervals)
            pcs.push_back ((root + interval) % 12);
        return pcs;
    }
}

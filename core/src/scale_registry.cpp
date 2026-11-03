#include "algo/ScaleRegistry.h"

#include "data_reader.h"

#include <numeric>
#include <stdexcept>
#include <utility>

namespace algo
{
    ScaleRegistry ScaleRegistry::fromJsonText (const std::string& text, const std::string& file)
    {
        ScaleRegistry registry;
        for (const auto& entry : detail::readEntries (text, file, "scales"))
        {
            ScaleDefinition def { entry.name (), entry.getIntList ("steps"), entry.getStringList ("tags") };

            for (int step : def.steps)
                if (step <= 0)
                    entry.fail ("each step must be larger than 0");

            const int sum = std::accumulate (def.steps.begin (), def.steps.end (), 0);
            if (sum != 12)
                entry.fail ("the steps add up to " + std::to_string (sum) + ", not 12");

            registry.m_index[detail::toLower (def.name)] = registry.m_scales.size ();
            registry.m_scales.push_back (std::move (def));
        }
        return registry;
    }

    const ScaleDefinition& ScaleRegistry::get (const std::string& name) const
    {
        const auto it = m_index.find (detail::toLower (name));
        if (it == m_index.end ())
            throw std::out_of_range ("Unknown scale: " + name);
        return m_scales[it->second];
    }

    bool ScaleRegistry::contains (const std::string& name) const
    {
        return m_index.count (detail::toLower (name)) > 0;
    }

    std::vector<std::string> ScaleRegistry::allNames () const
    {
        std::vector<std::string> names;
        names.reserve (m_scales.size ());
        for (const auto& def : m_scales)
            names.push_back (def.name);
        return names;
    }

    std::vector<PitchClass> ScaleRegistry::pitchClasses (PitchClass root, const ScaleDefinition& def)
    {
        std::vector<PitchClass> pcs;
        PitchClass              current = root % 12;
        for (int step : def.steps)
        {
            pcs.push_back (current);
            current = (current + step) % 12;
        }
        return pcs;
    }
}

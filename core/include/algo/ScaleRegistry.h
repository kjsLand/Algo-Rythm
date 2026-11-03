#pragma once

#include "Types.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace algo
{
    // A scale is defined by the semitone steps between consecutive degrees.
    // The steps add up to 12.
    struct ScaleDefinition
    {
        std::string              name;
        std::vector<int>         steps;
        std::vector<std::string> tags;
    };

    // The entries stay in file order. Lookup is not case-sensitive.
    class ScaleRegistry
    {
    public:
        // Throws DataError if the text is not valid.
        static ScaleRegistry fromJsonText (const std::string& text, const std::string& file = "scales.json");

        // Throws std::out_of_range if the name is unknown.
        const ScaleDefinition& get (const std::string& name) const;
        bool                   contains (const std::string& name) const;

        std::vector<std::string>            allNames () const;
        const std::vector<ScaleDefinition>& all () const
        {
            return m_scales;
        }

        // Returns the pitch classes in degree order. The first pitch class is the root.
        static std::vector<PitchClass> pitchClasses (PitchClass root, const ScaleDefinition& def);

    private:
        std::vector<ScaleDefinition>                 m_scales;
        std::unordered_map<std::string, std::size_t> m_index; // Lower-case name to index
    };
}

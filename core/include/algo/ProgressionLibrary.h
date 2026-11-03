#pragma once

#include "ChordRegistry.h"
#include "ScaleRegistry.h"
#include "Types.h"

#include <optional>
#include <string>
#include <vector>

namespace algo
{
    struct ProgressionDefinition
    {
        std::string              name;
        std::vector<std::string> numerals; // Nashville numbers relative to the scale of the mode
        std::string              mode;     // A name in the scale registry
        std::vector<std::string> tags;
        std::vector<std::string> roles; // "verse", "chorus", "bridge" or "loop"
        Difficulty               difficulty { Difficulty::Beginner };
    };

    // An empty or absent field matches all progressions.
    struct ProgressionFilter
    {
        std::string               mode;
        std::string               role;
        std::string               tag;
        std::optional<Difficulty> difficulty; // Also matches progressions of a lower difficulty
    };

    // The entries stay in file order.
    class ProgressionLibrary
    {
    public:
        // Throws DataError if the text is not valid or has an unknown scale or chord reference.
        static ProgressionLibrary fromJsonText (const std::string&   text,
                                                const ScaleRegistry& scales,
                                                const ChordRegistry& chords,
                                                const std::string&   file = "progressions.json");

        const std::vector<ProgressionDefinition>& all () const
        {
            return m_progressions;
        }

        // Returns the progressions that match all fields, in file order.
        std::vector<ProgressionDefinition> filter (const ProgressionFilter& filter) const;

    private:
        std::vector<ProgressionDefinition> m_progressions;
    };
}

#pragma once

#include "Types.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace algo
{
    // A chord is defined by its semitone intervals above the root.
    // The root is always implied at 0.
    struct ChordDefinition
    {
        std::string              name;   // For example "Major"
        std::string              symbol; // For example "", "m" or "maj7"
        std::vector<int>         intervals;
        std::vector<int>         optional; // The intervals that a voicing can omit
        std::vector<std::string> tags;
    };

    // The entries stay in file order. Lookup by name is not case-sensitive.
    class ChordRegistry
    {
    public:
        // Throws DataError if the text is not valid.
        static ChordRegistry fromJsonText (const std::string& text, const std::string& file = "chords.json");

        // Throws std::out_of_range if the name is unknown.
        const ChordDefinition& get (const std::string& name) const;
        bool                   contains (const std::string& name) const;

        // A symbol lookup is case-sensitive, because "m7" and "M7" are different symbols.
        bool containsSymbol (const std::string& symbol) const;

        // Throws std::out_of_range if the symbol is unknown.
        const ChordDefinition& getBySymbol (const std::string& symbol) const;

        std::vector<std::string>            allNames () const;
        const std::vector<ChordDefinition>& all () const
        {
            return m_chords;
        }

        // Returns the pitch classes of the chord. The first pitch class is the root.
        static std::vector<PitchClass> pitchClasses (PitchClass root, const ChordDefinition& def);

    private:
        std::vector<ChordDefinition>                 m_chords;
        std::unordered_map<std::string, std::size_t> m_index; // Lower-case name to index
        std::unordered_map<std::string, std::size_t> m_symbols;
    };
}

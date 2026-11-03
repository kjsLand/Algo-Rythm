#pragma once

#include <array>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <stdexcept>
#include <cctype>

// =============================================================================
// ChordDefinition
//
// A chord is defined as a set of semitone intervals above the root (the root
// itself is always implied at 0).
//
// Examples:
//   Major:   { 4, 7 }       root + maj3 + P5
//   Minor:   { 3, 7 }       root + min3 + P5
//   Maj7:    { 4, 7, 11 }
// =============================================================================

struct ChordDefinition
{
    std::string      name;      // Display name,  e.g. "Major"
    std::string      symbol;    // Short symbol,  e.g. "", "m", "maj7"
    std::vector<int> intervals; // Semitones above root (root=0 implied)
};

// =============================================================================
// ChordRegistry
// =============================================================================

class ChordRegistry
{
public:
    static ChordRegistry& instance ()
    {
        static ChordRegistry inst;
        return inst;
    }

    void registerChord (const ChordDefinition& def)
    {
        m_chords[normalise (def.name)] = def;
    }

    const ChordDefinition& get (const std::string& name) const
    {
        auto it = m_chords.find (normalise (name));
        if (it == m_chords.end ())
            throw std::out_of_range ("Unknown chord: " + name);
        return it->second;
    }

    bool contains (const std::string& name) const
    {
        return m_chords.count (normalise (name)) > 0;
    }

    std::vector<std::string> allNames () const
    {
        std::vector<std::string> names;
        names.reserve (m_chords.size ());
        for (auto& [k, v] : m_chords)
            names.push_back (v.name);
        return names;
    }

    // Returns the pitch classes (0-11) for a chord built on rootPC.
    static std::set<int> pitchClasses (int rootPC, const ChordDefinition& def)
    {
        std::set<int> pcs;
        pcs.insert (rootPC % 12);
        for (int iv : def.intervals)
            pcs.insert ((rootPC + iv) % 12);
        return pcs;
    }

    // Returns the note names for every chord tone, given a root pitch class.
    // Uses sharps throughout for consistency.
    static std::vector<std::string> noteNames (int rootPC, const ChordDefinition& def)
    {
        static const std::array<std::string, 12> CHROMATIC = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };

        std::vector<std::string> names;
        names.push_back (CHROMATIC[rootPC % 12]); // root
        for (int iv : def.intervals)
            names.push_back (CHROMATIC[(rootPC + iv) % 12]);
        return names;
    }

private:
    ChordRegistry ()
    {
        loadDefaults ();
    }

    void loadDefaults ()
    {
        // ---- Triads ----
        registerChord ({ "Major", "", { 4, 7 } });
        registerChord ({ "Minor", "m", { 3, 7 } });
        registerChord ({ "Aug", "aug", { 4, 8 } });
        registerChord ({ "Dim", "dim", { 3, 6 } });

        // ---- Seventh chords ----
        registerChord ({ "Maj7", "maj7", { 4, 7, 11 } });
        registerChord ({ "M7", "m7", { 3, 7, 10 } });
        registerChord ({ "Dom7", "7", { 4, 7, 10 } });
        registerChord ({ "Dim7", "dim7", { 3, 6, 9 } });
        registerChord ({ "M7b5", "m7b5", { 3, 6, 10 } }); // half-diminished

        // ---- Suspended ----
        registerChord ({ "Sus2", "sus2", { 2, 7 } });
        registerChord ({ "Sus4", "sus4", { 5, 7 } });

        // ---- Added-tone ----
        registerChord ({ "Add9", "add9", { 4, 7, 14 } }); // 14 = major 9th
    }

    static std::string normalise (std::string s)
    {
        for (char& c : s) c = static_cast<char> (std::tolower (static_cast<unsigned char> (c)));
        return s;
    }

    std::unordered_map<std::string, ChordDefinition> m_chords;
};
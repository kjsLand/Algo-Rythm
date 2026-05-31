#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <stdexcept>

// =============================================================================
// ScaleDefinition
//
// A scale is defined by its interval pattern: the semitone distances between
// each consecutive degree, starting from the root.
//
// Examples:
//   Major:           { 2, 2, 1, 2, 2, 2, 1 }   (W W H W W W H)
//   Natural Minor:   { 2, 1, 2, 2, 1, 2, 2 }
//   Pentatonic Maj:  { 2, 2, 3, 2, 3 }
// =============================================================================

struct ScaleDefinition
{
    std::string              name;      // Display name, e.g. "Major"
    std::vector<int>         intervals; // Semitone steps between degrees
};

// =============================================================================
// ScaleRegistry
//
// A singleton map from scale name -> ScaleDefinition.
// New scales can be registered at any time; lookup is case-insensitive.
// =============================================================================

class ScaleRegistry
{
public:
    static ScaleRegistry& instance()
    {
        static ScaleRegistry inst;
        return inst;
    }

    // Register (or overwrite) a scale
    void registerScale (const ScaleDefinition& def)
    {
        m_scales[normalise (def.name)] = def;
    }

    // Throws std::out_of_range if not found
    const ScaleDefinition& get (const std::string& name) const
    {
        auto it = m_scales.find (normalise (name));
        if (it == m_scales.end())
            throw std::out_of_range ("Unknown scale: " + name);
        return it->second;
    }

    bool contains (const std::string& name) const
    {
        return m_scales.count (normalise (name)) > 0;
    }

    std::vector<std::string> allNames() const
    {
        std::vector<std::string> names;
        names.reserve (m_scales.size());
        for (auto& [k, v] : m_scales)
            names.push_back (v.name);
        return names;
    }

    // Build a set of pitch classes (0-11) for a given root + scale
    // rootPitchClass: 0=C, 1=C#, 2=D, …, 11=B
    static std::set<int> pitchClasses (int rootPitchClass,
                                       const ScaleDefinition& def)
    {
        std::set<int> pcs;
        int current = rootPitchClass % 12;
        pcs.insert (current);
        for (int step : def.intervals)
        {
            current = (current + step) % 12;
            pcs.insert (current);
        }
        return pcs;
    }

private:
    ScaleRegistry() { loadDefaults(); }

    void loadDefaults()
    {
        // ---- Heptatonic (7-note) scales ----
        registerScale ({ "Major",
                         { 2, 2, 1, 2, 2, 2, 1 } });

        registerScale ({ "Natural Minor",
                         { 2, 1, 2, 2, 1, 2, 2 } });

        registerScale ({ "Harmonic Minor",
                         { 2, 1, 2, 2, 1, 3, 1 } });

        registerScale ({ "Melodic Minor",
                         { 2, 1, 2, 2, 2, 2, 1 } });

        registerScale ({ "Dorian",
                         { 2, 1, 2, 2, 2, 1, 2 } });

        registerScale ({ "Phrygian",
                         { 1, 2, 2, 2, 1, 2, 2 } });

        registerScale ({ "Lydian",
                         { 2, 2, 2, 1, 2, 2, 1 } });

        registerScale ({ "Mixolydian",
                         { 2, 2, 1, 2, 2, 1, 2 } });

        registerScale ({ "Locrian",
                         { 1, 2, 2, 1, 2, 2, 2 } });

        // ---- Pentatonic scales ----
        registerScale ({ "Major Pentatonic",
                         { 2, 2, 3, 2, 3 } });

        registerScale ({ "Minor Pentatonic",
                         { 3, 2, 2, 3, 2 } });

        // ---- Blues ----
        registerScale ({ "Blues",
                         { 3, 2, 1, 1, 3, 2 } });

        // ---- Whole tone ----
        registerScale ({ "Whole Tone",
                         { 2, 2, 2, 2, 2, 2 } });

        // ---- Diminished ----
        registerScale ({ "Diminished (HW)",
                         { 1, 2, 1, 2, 1, 2, 1, 2 } });

        registerScale ({ "Diminished (WH)",
                         { 2, 1, 2, 1, 2, 1, 2, 1 } });
    }

    static std::string normalise (std::string s)
    {
        // Lowercase for case-insensitive lookup
        for (char& c : s) c = static_cast<char> (std::tolower (static_cast<unsigned char> (c)));
        return s;
    }

    std::unordered_map<std::string, ScaleDefinition> m_scales;
};
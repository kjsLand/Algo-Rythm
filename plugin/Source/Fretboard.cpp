#include "Fretboard.h"

#include <cassert>
#include <cctype>
#include <algorithm>
#include <iostream>
#include <iomanip>

// =============================================================================
// Construction
// =============================================================================

GuitarFretboard::GuitarFretboard()
{
    buildGrid();
}

void GuitarFretboard::buildGrid()
{
    // Note names in chromatic order
    static const std::array<std::string, 12> NOTE_NAMES = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    for (int s = 0; s < NUM_STRINGS; ++s)
    {
        for (int f = 0; f < NUM_FRETS; ++f)
        {
            int midi = OPEN_MIDI[s] + f;

            FretNote note;
            note.midiNote = midi;
            note.name     = midiToName (midi);
            note.isOpen   = (f == 0);

            m_grid[s][f] = note;
        }
    }
}

// =============================================================================
// Access
// =============================================================================

const FretNote& GuitarFretboard::getNote (int s, int f) const
{
    assert (s >= 0 && s < NUM_STRINGS);
    assert (f >= 0 && f < NUM_FRETS);
    return m_grid[s][f];
}

int GuitarFretboard::getMidiNote (int s, int f) const
{
    return getNote (s, f).midiNote;
}

std::string GuitarFretboard::getNoteName (int s, int f) const
{
    return getNote (s, f).name;
}

// =============================================================================
// Search
// =============================================================================

std::vector<GuitarFretboard::Position> GuitarFretboard::findNote (int midiNote) const
{
    std::vector<Position> results;

    for (int s = 0; s < NUM_STRINGS; ++s)
        for (int f = 0; f < NUM_FRETS; ++f)
            if (m_grid[s][f].midiNote == midiNote)
                results.push_back ({ s, f });

    return results;
}

std::vector<GuitarFretboard::Position> GuitarFretboard::findNoteByName (const std::string& name) const
{
    // Case-insensitive prefix search: "a" matches "A2", "A#3", etc.
    auto toLower = [] (std::string s) {
        std::transform (s.begin(), s.end(), s.begin(),
                        [] (unsigned char c) { return std::tolower (c); });
        return s;
    };

    std::string needle = toLower (name);
    std::vector<Position> results;

    for (int s = 0; s < NUM_STRINGS; ++s)
    {
        for (int f = 0; f < NUM_FRETS; ++f)
        {
            std::string hay = toLower (m_grid[s][f].name);
            if (hay.find (needle) != std::string::npos)
                results.push_back ({ s, f });
        }
    }

    return results;
}

// =============================================================================
// Iteration
// =============================================================================

void GuitarFretboard::forEach (std::function<void(int, int, const FretNote&)> fn) const
{
    for (int s = 0; s < NUM_STRINGS; ++s)
        for (int f = 0; f < NUM_FRETS; ++f)
            fn (s, f, m_grid[s][f]);
}

// =============================================================================
// Debug
// =============================================================================

void GuitarFretboard::printGrid() const
{
    static const std::array<std::string, NUM_STRINGS> STRING_NAMES = {
        "E2", "A2", "D3", "G3", "B3", "E4"
    };

    // Header row
    std::cout << "       ";
    for (int f = 0; f < NUM_FRETS; ++f)
        std::cout << std::setw (5) << (f == 0 ? "Open" : std::to_string (f));
    std::cout << "\n";

    // Separator
    std::cout << "       " << std::string (NUM_FRETS * 5, '-') << "\n";

    // Each string (print high-to-low so it looks like a guitar neck)
    for (int s = NUM_STRINGS - 1; s >= 0; --s)
    {
        std::cout << std::setw (4) << STRING_NAMES[s] << " | ";
        for (int f = 0; f < NUM_FRETS; ++f)
            std::cout << std::setw (5) << m_grid[s][f].name;
        std::cout << "\n";
    }
}

// =============================================================================
// Private helpers
// =============================================================================

std::string GuitarFretboard::midiToName (int midiNote)
{
    static const std::array<std::string, 12> CHROMATIC = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    // MIDI note 0 = C-1; octave numbering: C4 = MIDI 60
    int octave    = (midiNote / 12) - 1;
    int semitone  = midiNote % 12;

    return CHROMATIC[semitone] + std::to_string (octave);
}
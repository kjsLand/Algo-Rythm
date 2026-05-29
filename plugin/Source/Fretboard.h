#pragma once

#include <array>
#include <string>
#include <optional>
#include <functional>

// Represents a single note on the fretboard
struct FretNote
{
    int midiNote;       // MIDI note number (0-127)
    std::string name;   // e.g. "E2", "A2", "D3"
    bool isOpen;        // true if this is an open string (fret 0)
};

// GuitarFretboard models a standard 6-string guitar, frets 0-12.
// String index 0 = low E (E2), string index 5 = high e (E4).
// Fret  index 0 = open string, fret index 1-12 = frets 1-12.
class GuitarFretboard
{
public:
    static constexpr int NUM_STRINGS = 6;
    static constexpr int NUM_FRETS   = 13; // 0 (open) + frets 1-12

    GuitarFretboard();

    // -------------------------------------------------------------------------
    // Access
    // -------------------------------------------------------------------------

    // Returns the note at [string][fret]. Bounds-checked via assert in debug.
    const FretNote& getNote (int stringIndex, int fretIndex) const;

    // Returns the MIDI note number at [string][fret].
    int getMidiNote (int stringIndex, int fretIndex) const;

    // Returns a human-readable name, e.g. "E2", for the note at [string][fret].
    std::string getNoteName (int stringIndex, int fretIndex) const;

    // -------------------------------------------------------------------------
    // Search
    // -------------------------------------------------------------------------

    // Returns every (string, fret) position that matches the given MIDI note.
    struct Position { int string; int fret; };
    std::vector<Position> findNote (int midiNote) const;

    // Returns every (string, fret) position whose note name contains `name`
    // (case-insensitive), e.g. "A" matches "A2", "A3", "A#3", etc.
    std::vector<Position> findNoteByName (const std::string& name) const;

    // -------------------------------------------------------------------------
    // Iteration
    // -------------------------------------------------------------------------

    // Calls fn(stringIndex, fretIndex, FretNote) for every cell in the matrix.
    void forEach (std::function<void(int, int, const FretNote&)> fn) const;

    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------

    // Prints the full 6x13 grid to stdout (useful for debugging).
    void printGrid() const;

private:
    // [string][fret]
    std::array<std::array<FretNote, NUM_FRETS>, NUM_STRINGS> m_grid;

    void buildGrid();

    // Standard tuning open-string MIDI notes (E2 A2 D3 G3 B3 E4)
    static constexpr std::array<int, NUM_STRINGS> OPEN_MIDI = { 40, 45, 50, 55, 59, 64 };

    static std::string midiToName (int midiNote);
};
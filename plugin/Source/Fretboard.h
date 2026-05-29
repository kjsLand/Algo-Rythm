#pragma once

#include "ScaleRegistry.h"

#include <array>
#include <string>
#include <vector>
#include <functional>
#include <set>

// =============================================================================
// FretNote
// =============================================================================

struct FretNote
{
    int         midiNote = 0;
    std::string name;       // e.g. "A3", "C#4"
    bool        isOpen = false;
};

// =============================================================================
// GuitarFretboard
// =============================================================================

class GuitarFretboard
{
public:
    // Standard 6-string guitar, EADGBE, 24 frets (fret 0 = open string)
    static constexpr int NUM_STRINGS = 6;
    static constexpr int NUM_FRETS   = 25; // 0-24

    // MIDI note numbers for open strings (low E to high E)
    static constexpr std::array<int, NUM_STRINGS> OPEN_MIDI = { 40, 45, 50, 55, 59, 64 };

    struct Position { int string; int fret; };

    GuitarFretboard();

    // -------------------------------------------------------------------------
    // Basic access
    // -------------------------------------------------------------------------
    const FretNote& getNote      (int string, int fret) const;
    int             getMidiNote  (int string, int fret) const;
    std::string     getNoteName  (int string, int fret) const;

    // -------------------------------------------------------------------------
    // Search
    // -------------------------------------------------------------------------
    std::vector<Position> findNote       (int midiNote)              const;
    std::vector<Position> findNoteByName (const std::string& name)   const;

    // -------------------------------------------------------------------------
    // Iteration
    // -------------------------------------------------------------------------
    void forEach (std::function<void(int, int, const FretNote&)> fn) const;

    // -------------------------------------------------------------------------
    // Scale utilities
    // -------------------------------------------------------------------------

    // Returns the set of pitch classes (0-11) that belong to the given scale.
    // key:       note name without octave, e.g. "C", "F#", "Bb"
    // scaleName: must exist in ScaleRegistry, e.g. "Major", "Dorian"
    static std::set<int> getScalePitchClasses (const std::string& key,
                                               const std::string& scaleName);

    // Returns all fretboard positions whose pitch class is in the scale.
    std::vector<Position> getScalePositions (const std::string& key,
                                             const std::string& scaleName) const;

    // Prints the fretboard grid showing only notes in the requested scale.
    //
    //  key       - root note, e.g. "C", "G", "F#", "Bb"
    //  scaleName - scale name from the registry, e.g. "Major", "Natural Minor"
    //  maxFret   - highest fret column to print (default: 12)
    //
    // Cells that belong to the scale show the note name.
    // The root note is marked with [brackets].
    // All other cells show a dash.
    void printScale (const std::string& key,
                     const std::string& scaleName,
                     int maxFret = 12) const;

    // Prints the full chromatic grid (original behaviour).
    void printGrid() const;

    // -------------------------------------------------------------------------
    // Convenience: list available scale names
    // -------------------------------------------------------------------------
    static std::vector<std::string> availableScales();

private:
    std::array<std::array<FretNote, NUM_FRETS>, NUM_STRINGS> m_grid;

    void        buildGrid();
    static std::string midiToName  (int midiNote);
    static int         nameToPC    (const std::string& noteName); // pitch class 0-11
};
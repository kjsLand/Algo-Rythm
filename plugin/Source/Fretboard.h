#pragma once

#include "ScaleRegistry.h"
#include "ChordRegistry.h"

#include <array>
#include <string>
#include <vector>
#include <functional>
#include <set>
#include <random>
#include <optional>

// =============================================================================
// FretNote
// =============================================================================

struct FretNote
{
    int         midiNote = 0;
    std::string name; // e.g. "A3", "C#4"
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

    struct Position
    {
        int string;
        int fret;
    };

    GuitarFretboard ();

    // -------------------------------------------------------------------------
    // Basic access
    // -------------------------------------------------------------------------
    const FretNote& getNote (int string, int fret) const;
    int             getMidiNote (int string, int fret) const;
    std::string     getNoteName (int string, int fret) const;

    // -------------------------------------------------------------------------
    // Search
    // -------------------------------------------------------------------------
    std::vector<Position> findNote (int midiNote) const;
    std::vector<Position> findNoteByName (const std::string& name) const;

    // -------------------------------------------------------------------------
    // Iteration
    // -------------------------------------------------------------------------
    void forEach (std::function<void (int, int, const FretNote&)> fn) const;

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
                     int                maxFret = 12) const;

    // Prints the full chromatic grid (original behaviour).
    void printGrid () const;

    // -------------------------------------------------------------------------
    // Chord utilities
    // -------------------------------------------------------------------------

    // Given a root pitch class and a chord name, return the note name strings
    // for every chord tone, e.g. { "A", "C", "E" } for A minor.
    static std::vector<std::string> getChordNotes (int                rootPC,
                                                   const std::string& chordName);

    // Print all requested chord types built on a given root pitch class.
    // chordNames: list of chord names from ChordRegistry
    //             (pass an empty vector to print all registered chords)
    static void printChords (int                             rootPC,
                             const std::vector<std::string>& chordNames = {});

    // -------------------------------------------------------------------------
    // Random scale-note picker
    // -------------------------------------------------------------------------

    // ChordSuggestion bundles a picked fretboard position with chord data for
    // every chord type built on that note.
    struct ChordNote
    {
        std::string              chordName; // e.g. "Major"
        std::string              symbol;    // e.g. "m7"
        std::vector<std::string> noteNames; // e.g. { "A", "C", "E", "G" }
    };

    struct ScaleNotePick
    {
        Position               position;   // where on the neck
        std::string            noteName;   // e.g. "A"
        int                    pitchClass; // 0-11
        std::vector<ChordNote> chords;     // one entry per requested chord type
    };

    // Pick `count` random positions from the scale and return chord suggestions.
    //
    //  key        - scale root, e.g. "A"
    //  scaleName  - scale from ScaleRegistry, e.g. "Major"
    //  chordNames - chord types to build on each picked note
    //               (pass {} for the default set used in the task:
    //                Major, Minor, Maj7, M7, Dom7, Sus2, Sus4, Add9, Dim, Dim7, M7b5)
    //  count      - how many notes to pick (default 1)
    //  seed       - RNG seed; use std::nullopt for a random seed
    std::vector<ScaleNotePick> pickRandomScaleNotes (
        const std::string&              key,
        const std::string&              scaleName,
        const std::vector<std::string>& chordNames = {},
        int                             count      = 1,
        std::optional<uint32_t>         seed       = std::nullopt) const;

    // Pretty-print the result of pickRandomScaleNotes().
    static void printScaleNotePicks (const std::vector<ScaleNotePick>& picks);

    // -------------------------------------------------------------------------
    // Chord voicing
    // -------------------------------------------------------------------------

    // A voiced chord: one fretboard Position per chord tone, chosen to be the
    // closest fret to rootPos (tight voicing).  The first entry is always the
    // root itself.
    struct VoicedChord
    {
        std::string           chordName; // e.g. "Major"
        std::string           symbol;    // e.g. "Gmaj7"
        std::vector<Position> positions; // one per tone, index 0 = root
    };

    // Build a tight voicing: for each chord tone (pitch class), pick the single
    // fretboard position (fret 0-12) whose fret is closest to rootPos.fret.
    // Ties broken by lowest string index.
    VoicedChord buildTightVoicing (const Position&    rootPos,
                                   const std::string& chordName) const;

    // -------------------------------------------------------------------------
    // Convenience: list available scale / chord names
    // -------------------------------------------------------------------------
    static std::vector<std::string> availableScales ();
    static std::vector<std::string> availableChords ();

private:
    std::array<std::array<FretNote, NUM_FRETS>, NUM_STRINGS> m_grid;

    void               buildGrid ();
    static std::string midiToName (int midiNote);
    static int         nameToPC (const std::string& noteName); // pitch class 0-11
};
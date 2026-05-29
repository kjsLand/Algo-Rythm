#include "Fretboard.h"

#include <cassert>
#include <cctype>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <optional>
#include <random>

// =============================================================================
// Construction
// =============================================================================

GuitarFretboard::GuitarFretboard()
{
    buildGrid();
}

void GuitarFretboard::buildGrid()
{
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
// Scale utilities
// =============================================================================

// ---------------------------------------------------------------------------
// nameToPC  –  converts a note name (no octave) to a pitch class 0-11.
//              Accepts sharps (#) and flats (b).  Case-insensitive.
// ---------------------------------------------------------------------------
int GuitarFretboard::nameToPC (const std::string& noteName)
{
    if (noteName.empty())
        throw std::invalid_argument ("Empty note name");

    // Normalise first character to uppercase
    char letter = static_cast<char> (std::toupper (static_cast<unsigned char> (noteName[0])));

    static const std::string NATURALS = "CDEFGAB";
    static const int         PC[]     = { 0, 2, 4, 5, 7, 9, 11 }; // C D E F G A B

    auto pos = NATURALS.find (letter);
    if (pos == std::string::npos)
        throw std::invalid_argument ("Unknown note letter: " + noteName);

    int pc = PC[pos];

    // Apply accidentals (rest of string)
    for (std::size_t i = 1; i < noteName.size(); ++i)
    {
        char acc = noteName[i];
        if (acc == '#') pc = (pc + 1) % 12;
        else if (acc == 'b') pc = (pc + 11) % 12;
        else break; // stop on octave digit or anything else
    }

    return pc;
}

std::set<int> GuitarFretboard::getScalePitchClasses (const std::string& key,
                                                     const std::string& scaleName)
{
    const ScaleDefinition& def = ScaleRegistry::instance().get (scaleName);
    int rootPC = nameToPC (key);
    return ScaleRegistry::pitchClasses (rootPC, def);
}

std::vector<GuitarFretboard::Position>
GuitarFretboard::getScalePositions (const std::string& key,
                                    const std::string& scaleName) const
{
    std::set<int> pcs = getScalePitchClasses (key, scaleName);
    std::vector<Position> results;

    for (int s = 0; s < NUM_STRINGS; ++s)
        for (int f = 0; f < NUM_FRETS; ++f)
            if (pcs.count (m_grid[s][f].midiNote % 12))
                results.push_back ({ s, f });

    return results;
}

// ---------------------------------------------------------------------------
// printScale
// ---------------------------------------------------------------------------

void GuitarFretboard::printScale (const std::string& key,
                                  const std::string& scaleName,
                                  int maxFret) const
{
    // Clamp maxFret to grid size
    maxFret = std::min (maxFret, NUM_FRETS - 1);

    const std::set<int> pcs    = getScalePitchClasses (key, scaleName);
    const int           rootPC = nameToPC (key);

    // Column width: wide enough for "[C#]" (4 chars) plus padding
    constexpr int COL = 6;

    static const std::array<std::string, NUM_STRINGS> STRING_NAMES = {
        "E2", "A2", "D3", "G3", "B3", "E4"
    };

    // ---- Header ----
    const ScaleDefinition& def = ScaleRegistry::instance().get (scaleName);

    // Collect the scale degree names for the legend
    std::string scaleNotes;
    {
        static const std::array<std::string, 12> CHROMATIC = {
            "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
        };
        int cur = rootPC;
        scaleNotes += CHROMATIC[cur];
        for (int step : def.intervals)
        {
            cur = (cur + step) % 12;
            if (cur == rootPC) break; // avoid duplicating root at octave
            scaleNotes += " - " + CHROMATIC[cur];
        }
    }

    std::cout << "\n";
    std::cout << "  Scale : " << key << " " << def.name << "\n";
    std::cout << "  Notes : " << scaleNotes << "\n";
    std::cout << "\n";

    // ---- Fret number row ----
    std::cout << "        "; // string-name indent
    for (int f = 0; f <= maxFret; ++f)
    {
        std::string label = (f == 0) ? "Open" : std::to_string (f);
        std::cout << std::setw (COL) << label;
    }
    std::cout << "\n";

    // ---- Separator ----
    std::cout << "        " << std::string ((maxFret + 1) * COL, '-') << "\n";

    // ---- String rows (high → low, like looking at a guitar neck) ----
    for (int s = NUM_STRINGS - 1; s >= 0; --s)
    {
        std::cout << std::setw (4) << STRING_NAMES[s] << " | ";

        for (int f = 0; f <= maxFret; ++f)
        {
            const FretNote& note = m_grid[s][f];
            int pc = note.midiNote % 12;

            if (pcs.count (pc))
            {
                // Strip octave digit(s) from name for a tidy display
                std::string notePart;
                for (char c : note.name)
                    if (!std::isdigit (static_cast<unsigned char> (c)) && c != '-')
                        notePart += c;

                std::string cell;
                if (pc == rootPC)
                    cell = "[" + notePart + "]"; // mark root
                else
                    cell = notePart;

                std::cout << std::setw (COL) << cell;
            }
            else
            {
                std::cout << std::setw (COL) << ".";
            }
        }

        std::cout << "\n";
    }

    // ---- Inlay dot reminder ----
    std::cout << "        ";
    for (int f = 0; f <= maxFret; ++f)
    {
        static const std::set<int> INLAYS = { 3, 5, 7, 9, 12 };
        std::cout << std::setw (COL) << (INLAYS.count (f) ? "|" : " ");
    }
    std::cout << "\n\n";
}

// =============================================================================
// Chord utilities
// =============================================================================

std::vector<std::string> GuitarFretboard::getChordNotes (int rootPC,
                                                         const std::string& chordName)
{
    const ChordDefinition& def = ChordRegistry::instance().get (chordName);
    return ChordRegistry::noteNames (rootPC, def);
}

void GuitarFretboard::printChords (int rootPC,
                                   const std::vector<std::string>& chordNames)
{
    static const std::array<std::string, 12> CHROMATIC = {
        "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
    };

    const std::string rootName = CHROMATIC[rootPC % 12];

    // Resolve which chords to print
    std::vector<std::string> names = chordNames.empty()
                                     ? ChordRegistry::instance().allNames()
                                     : chordNames;

    constexpr int NAME_W   = 10;
    constexpr int SYMBOL_W = 8;

    std::cout << "\n  Chords on root: " << rootName << "\n";
    std::cout << "  " << std::string (50, '-') << "\n";
    std::cout << "  " << std::left
              << std::setw (NAME_W)   << "Type"
              << std::setw (SYMBOL_W) << "Symbol"
              << "Notes\n";
    std::cout << "  " << std::string (50, '-') << "\n";

    for (const auto& cname : names)
    {
        const ChordDefinition& def  = ChordRegistry::instance().get (cname);
        auto                   tones = ChordRegistry::noteNames (rootPC, def);

        std::string notesStr;
        for (std::size_t i = 0; i < tones.size(); ++i)
        {
            if (i) notesStr += " - ";
            notesStr += tones[i];
        }

        std::string fullSymbol = rootName + def.symbol;

        std::cout << "  " << std::left
                  << std::setw (NAME_W)   << def.name
                  << std::setw (SYMBOL_W) << fullSymbol
                  << notesStr << "\n";
    }

    std::cout << "\n";
}

// =============================================================================
// Random scale-note picker
// =============================================================================

// Default chord set for the task
static const std::vector<std::string> DEFAULT_CHORD_TYPES = {
    "Major", "Minor", "Maj7", "M7", "Dom7",
    "Sus2",  "Sus4",  "Add9", "Dim", "Dim7", "M7b5"
};

std::vector<GuitarFretboard::ScaleNotePick>
GuitarFretboard::pickRandomScaleNotes (const std::string&             key,
                                       const std::string&             scaleName,
                                       const std::vector<std::string>& chordNames,
                                       int                            count,
                                       std::optional<uint32_t>        seed) const
{
    // Gather all in-scale positions
    std::vector<Position> pool = getScalePositions (key, scaleName);
    if (pool.empty())
        return {};

    // Set up RNG
    std::mt19937 rng;
    if (seed.has_value())
        rng.seed (*seed);
    else
        rng.seed (std::random_device{}());

    const std::vector<std::string>& types =
        chordNames.empty() ? DEFAULT_CHORD_TYPES : chordNames;

    static const std::array<std::string, 12> CHROMATIC = {
        "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
    };

    std::vector<ScaleNotePick> picks;
    picks.reserve (count);

    std::uniform_int_distribution<int> dist (0, static_cast<int> (pool.size()) - 1);

    for (int i = 0; i < count; ++i)
    {
        Position pos = pool[dist (rng)];
        int midi     = m_grid[pos.string][pos.fret].midiNote;
        int pc       = midi % 12;

        ScaleNotePick pick;
        pick.position   = pos;
        pick.noteName   = CHROMATIC[pc];
        pick.pitchClass = pc;

        for (const auto& cname : types)
        {
            const ChordDefinition& def = ChordRegistry::instance().get (cname);
            ChordNote cn;
            cn.chordName = def.name;
            cn.symbol    = CHROMATIC[pc] + def.symbol;
            cn.noteNames = ChordRegistry::noteNames (pc, def);
            pick.chords.push_back (std::move (cn));
        }

        picks.push_back (std::move (pick));
    }

    return picks;
}

void GuitarFretboard::printScaleNotePicks (const std::vector<ScaleNotePick>& picks)
{
    static const std::array<std::string, NUM_STRINGS> STRING_NAMES = {
        "E2", "A2", "D3", "G3", "B3", "E4"
    };

    constexpr int NAME_W   = 10;
    constexpr int SYMBOL_W = 8;

    for (std::size_t i = 0; i < picks.size(); ++i)
    {
        const auto& pick = picks[i];

        std::cout << "\n";
        std::cout << "  ┌─ Pick #" << (i + 1) << " ─────────────────────────────────\n";
        std::cout << "  │  Note  : " << pick.noteName << "\n";
        std::cout << "  │  String: " << STRING_NAMES[pick.position.string]
                  << "   Fret: "     << pick.position.fret
                  << (pick.position.fret == 0 ? " (open)" : "") << "\n";
        std::cout << "  ├─────────────────────────────────────────────\n";
        std::cout << "  │  " << std::left
                  << std::setw (NAME_W)   << "Type"
                  << std::setw (SYMBOL_W) << "Symbol"
                  << "Notes\n";
        std::cout << "  ├─────────────────────────────────────────────\n";

        for (const auto& cn : pick.chords)
        {
            std::string notesStr;
            for (std::size_t j = 0; j < cn.noteNames.size(); ++j)
            {
                if (j) notesStr += " - ";
                notesStr += cn.noteNames[j];
            }

            std::cout << "  │  " << std::left
                      << std::setw (NAME_W)   << cn.chordName
                      << std::setw (SYMBOL_W) << cn.symbol
                      << notesStr << "\n";
        }

        std::cout << "  └─────────────────────────────────────────────\n";
    }

    std::cout << "\n";
}

// =============================================================================
// Available scales / chords
// =============================================================================

std::vector<std::string> GuitarFretboard::availableScales()
{
    return ScaleRegistry::instance().allNames();
}

std::vector<std::string> GuitarFretboard::availableChords()
{
    return ChordRegistry::instance().allNames();
}

// =============================================================================
// Debug – full chromatic grid
// =============================================================================

void GuitarFretboard::printGrid() const
{
    static const std::array<std::string, NUM_STRINGS> STRING_NAMES = {
        "E2", "A2", "D3", "G3", "B3", "E4"
    };

    std::cout << "       ";
    for (int f = 0; f < NUM_FRETS; ++f)
        std::cout << std::setw (5) << (f == 0 ? "Open" : std::to_string (f));
    std::cout << "\n";

    std::cout << "       " << std::string (NUM_FRETS * 5, '-') << "\n";

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

    int octave   = (midiNote / 12) - 1;
    int semitone = midiNote % 12;

    return CHROMATIC[semitone] + std::to_string (octave);
}
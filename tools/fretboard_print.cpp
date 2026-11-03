// Prints the fretboard grid, one scale and the chords on the root of the scale.
// Build from the repository root: cmake --build build --target fretboard_print
// Usage: fretboard_print [key] [scale] [capo]
#include "algo/Fretboard.h"
#include "algo/Pitch.h"
#include "algo/Scale.h"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>

namespace
{
    using namespace algo;

    // The note of the string with no capo. It does not depend on the key.
    std::string stringLabel (const Fretboard& board, int string)
    {
        return midiNoteName (board.tuning ().strings[static_cast<std::size_t> (string)], Key {});
    }

    // Prints the fretboard grid showing only notes in the scale.
    // The root note is marked with [brackets]. All other cells show a dot.
    void printScale (const Fretboard& board, const Key& key, int maxFret = 12)
    {
        maxFret = std::min (maxFret, board.numFrets () - 1);

        // Column width: wide enough for "[C#]" (4 chars) plus padding
        constexpr int COL = 6;

        const auto scale = scalePitchClasses (key);

        std::string scaleNotes;
        for (PitchClass pc : scale)
            scaleNotes += (scaleNotes.empty () ? "" : " - ") + pitchClassName (pc, key);

        std::cout << "\n";
        std::cout << "  Scale : " << pitchClassName (key.root, key) << " " << key.scale << "\n";
        std::cout << "  Notes : " << scaleNotes << "\n";
        std::cout << "\n";

        // ---- Fret number row ----
        std::cout << "        ";
        for (int f = 0; f <= maxFret; ++f)
            std::cout << std::setw (COL) << (f == 0 ? "Open" : std::to_string (f));
        std::cout << "\n";

        std::cout << "        " << std::string (static_cast<std::size_t> ((maxFret + 1) * COL), '-') << "\n";

        // ---- String rows (high to low, like looking at a guitar neck) ----
        for (int s = kNumStrings - 1; s >= 0; --s)
        {
            std::cout << std::setw (4) << stringLabel (board, s) << " | ";

            for (int f = 0; f <= maxFret; ++f)
            {
                const PitchClass pc = pitchClassOf (board.midiNoteAt (s, f));

                std::string cell = ".";
                if (isInScale (pc, key))
                {
                    cell = pitchClassName (pc, key);
                    if (pc == pitchClassOf (key.root))
                        cell = "[" + cell + "]";
                }

                std::cout << std::setw (COL) << cell;
            }

            std::cout << "\n";
        }

        // ---- Inlay dot reminder ----
        static const std::set<int> INLAYS = { 3, 5, 7, 9, 12 };

        std::cout << "        ";
        for (int f = 0; f <= maxFret; ++f)
            std::cout << std::setw (COL) << (INLAYS.count (f) ? "|" : " ");
        std::cout << "\n\n";
    }

    // Prints the full chromatic grid.
    void printGrid (const Fretboard& board, const Key& key)
    {
        std::cout << "       ";
        for (int f = 0; f < board.numFrets (); ++f)
            std::cout << std::setw (5) << (f == 0 ? "Open" : std::to_string (f));
        std::cout << "\n";

        std::cout << "       " << std::string (static_cast<std::size_t> (board.numFrets () * 5), '-') << "\n";

        for (int s = kNumStrings - 1; s >= 0; --s)
        {
            std::cout << std::setw (4) << stringLabel (board, s) << " | ";
            for (int f = 0; f < board.numFrets (); ++f)
                std::cout << std::setw (5) << board.noteNameAt (s, f, key);
            std::cout << "\n";
        }
    }

    // Prints all chord types in the registry on the root of the key.
    void printChords (const Key& key)
    {
        constexpr int NAME_W   = 10;
        constexpr int SYMBOL_W = 10;

        const std::string rootName = pitchClassName (key.root, key);

        std::cout << "\n  Chords on root: " << rootName << "\n";
        std::cout << "  " << std::string (50, '-') << "\n";
        std::cout << "  " << std::left
                  << std::setw (NAME_W) << "Type"
                  << std::setw (SYMBOL_W) << "Symbol"
                  << "Notes\n";
        std::cout << "  " << std::string (50, '-') << "\n";

        for (const auto& def : bundledData ().chords.all ())
        {
            std::string notes;
            for (PitchClass pc : ChordRegistry::pitchClasses (key.root, def))
                notes += (notes.empty () ? "" : " - ") + pitchClassName (pc, key);

            std::cout << "  " << std::left
                      << std::setw (NAME_W) << def.name
                      << std::setw (SYMBOL_W) << rootName + def.symbol
                      << notes << "\n";
        }

        std::cout << std::right << "\n";
    }
}

int main (int argc, char* argv[])
{
    try
    {
        const Key       key { parsePitchClass (argc > 1 ? argv[1] : "C"), argc > 2 ? argv[2] : "Major" };
        const Fretboard board (Tuning {}, argc > 3 ? std::stoi (argv[3]) : 0);

        printGrid (board, key);
        printScale (board, key);
        printChords (key);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what () << "\n";
        return 1;
    }

    return 0;
}

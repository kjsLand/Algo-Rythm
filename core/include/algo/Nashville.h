#pragma once

#include "Data.h"
#include "Types.h"

#include <string>

namespace algo
{
    // The quality of a chord symbol that takes its chord from the scale.
    inline constexpr const char* kDiatonicQuality = "diatonic";

    // Accepts "1", "6m", "b7", "4maj7", "5/7" and "#4dim". A numeral with no quality is "diatonic".
    // This function does not look up the quality. Throws std::invalid_argument if the format is not valid.
    ChordSymbol parseNumeral (const std::string& text);

    // Replaces a "diatonic" quality with the symbol of the diatonic triad on the degree.
    // A degree with an accidental has no diatonic chord, so it becomes a major chord.
    // Throws std::invalid_argument if the quality is not in the chord registry.
    ChordSymbol resolve (const ChordSymbol& symbol, const Key& key, const DataSet& data = bundledData ());

    // Applies the accidental to the scale degree. Throws std::invalid_argument
    // if the scale does not have 7 notes or the degree is not valid.
    PitchClass chordRootPc (const ChordSymbol& symbol, const Key& key, const DataSet& data = bundledData ());

    // True if all notes of the chord, and its slash bass note, are in the scale.
    // Throws as resolve and chordRootPc do.
    bool isDiatonic (const ChordSymbol& symbol, const Key& key, const DataSet& data = bundledData ());

    // Returns text such as "Em", "Bb" or "D/F#". A note with an accidental
    // follows the accidental and not the key, so "b7" in C major is "Bb".
    std::string chordName (const ChordSymbol& symbol, const Key& key, const DataSet& data = bundledData ());

    // Returns text such as "6m" or "b7". It parses back to the same degree, quality and bass.
    std::string numeralText (const ChordSymbol& symbol);
}

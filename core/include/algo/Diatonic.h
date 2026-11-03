#pragma once

#include "Data.h"
#include "Types.h"

namespace algo
{
    struct DiatonicChord
    {
        PitchClass      root { 0 };
        ChordDefinition chord;
    };

    // Returns the pitch class at a degree from 1 to 7.
    // Throws std::invalid_argument if the scale does not have 7 notes or the degree is not valid.
    PitchClass degreePitchClass (const Key& key, int degree, const DataSet& data = bundledData ());

    // Uses the scale notes at degree d, d+2 and d+4.
    // Throws as degreePitchClass does. Throws DataError if no chord type has the same intervals.
    DiatonicChord diatonicTriad (const Key& key, int degree, const DataSet& data = bundledData ());

    // Also uses the scale note at degree d+6. Throws as diatonicTriad does.
    DiatonicChord diatonicSeventh (const Key& key, int degree, const DataSet& data = bundledData ());
}

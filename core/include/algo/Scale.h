#pragma once

#include "Data.h"
#include "Types.h"

#include <optional>
#include <vector>

namespace algo
{
    // The functions in this file throw std::out_of_range if the key has an unknown scale.

    // Returns the pitch classes in degree order. The first pitch class is the root.
    std::vector<PitchClass> scalePitchClasses (const Key& key, const DataSet& data = bundledData ());

    // Returns the degree, from 1 to the number of notes in the scale, or no value if the note is not in the scale.
    std::optional<int> scaleDegreeOf (PitchClass pc, const Key& key, const DataSet& data = bundledData ());

    bool isInScale (PitchClass pc, const Key& key, const DataSet& data = bundledData ());

    // If two notes have the same distance, returns the lower note.
    MidiNote nearestScaleNote (MidiNote midiNote, const Key& key, const DataSet& data = bundledData ());
}

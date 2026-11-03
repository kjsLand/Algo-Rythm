#include "algo/Scale.h"

#include "algo/Pitch.h"

#include <algorithm>

namespace algo
{
    std::vector<PitchClass> scalePitchClasses (const Key& key, const DataSet& data)
    {
        return ScaleRegistry::pitchClasses (pitchClassOf (key.root), data.scales.get (key.scale));
    }

    std::optional<int> scaleDegreeOf (PitchClass pc, const Key& key, const DataSet& data)
    {
        const auto scale = scalePitchClasses (key, data);
        const auto it    = std::find (scale.begin (), scale.end (), pitchClassOf (pc));
        if (it == scale.end ())
            return std::nullopt;
        return static_cast<int> (it - scale.begin ()) + 1;
    }

    bool isInScale (PitchClass pc, const Key& key, const DataSet& data)
    {
        return scaleDegreeOf (pc, key, data).has_value ();
    }

    MidiNote nearestScaleNote (MidiNote midiNote, const Key& key, const DataSet& data)
    {
        const auto scale   = scalePitchClasses (key, data);
        const auto inScale = [&scale] (MidiNote note)
        {
            return std::find (scale.begin (), scale.end (), pitchClassOf (note)) != scale.end ();
        };

        // The lower note comes first, so it wins if two notes have the same distance.
        // The scale contains its root, so the search always finds a note in one octave.
        int distance = 0;
        while (!inScale (midiNote - distance) && !inScale (midiNote + distance))
            ++distance;

        return inScale (midiNote - distance) ? midiNote - distance : midiNote + distance;
    }

    bool isMinorScale (const std::string& scale, const DataSet& data)
    {
        const auto& tags = data.scales.get (scale).tags;
        return std::find (tags.begin (), tags.end (), "minor") != tags.end ();
    }
}

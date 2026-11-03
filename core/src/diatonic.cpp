#include "algo/Diatonic.h"

#include "algo/DataError.h"
#include "algo/Pitch.h"
#include "algo/Scale.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace algo
{
    namespace
    {
        constexpr int kDegrees = 7;

        DiatonicChord stackThirds (const Key& key, int degree, int noteCount, const DataSet& data)
        {
            const PitchClass root  = degreePitchClass (key, degree, data);
            const auto       scale = scalePitchClasses (key, data);

            std::vector<int> intervals;
            std::string      text;
            for (int i = 1; i < noteCount; ++i)
            {
                const int note = scale[static_cast<std::size_t> ((degree - 1 + 2 * i) % kDegrees)];
                intervals.push_back (pitchClassOf (note - root));
                text += (text.empty () ? "" : ", ") + std::to_string (intervals.back ());
            }

            for (const auto& chord : data.chords.all ())
                if (chord.intervals == intervals)
                    return { root, chord };

            throw DataError ("chords.json", "", "no chord type has the intervals " + text);
        }
    }

    PitchClass degreePitchClass (const Key& key, int degree, const DataSet& data)
    {
        const auto scale = scalePitchClasses (key, data);
        if (scale.size () != kDegrees)
            throw std::invalid_argument ("The scale '" + key.scale + "' does not have 7 notes");
        if (degree < 1 || degree > kDegrees)
            throw std::invalid_argument ("The degree must be 1 to 7, not " + std::to_string (degree));

        return scale[static_cast<std::size_t> (degree - 1)];
    }

    DiatonicChord diatonicTriad (const Key& key, int degree, const DataSet& data)
    {
        return stackThirds (key, degree, 3, data);
    }

    DiatonicChord diatonicSeventh (const Key& key, int degree, const DataSet& data)
    {
        return stackThirds (key, degree, 4, data);
    }
}

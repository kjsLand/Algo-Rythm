#include "algo/Pitch.h"
#include "algo/Voicing.h"

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace algo
{
    std::vector<std::string> violations (const Voicing& voicing, const ConcreteChord& chord, Difficulty difficulty, const Fretboard& fretboard, bool strummed)
    {
        for (int fret : voicing.frets)
            if (fret != kMutedString && (fret < 0 || fret >= fretboard.numFrets ()))
                return { kRuleOneNotePerString };

        std::vector<std::string> broken;

        std::set<PitchClass> sounding;
        MidiNote             lowest      = 128;
        int                  soundCount  = 0;
        int                  firstString = -1;
        int                  lastString  = -1;
        int                  lowestFret  = 0;
        int                  highestFret = 0;

        for (int s = 0; s < kNumStrings; ++s)
        {
            const int fret = voicing.frets[static_cast<std::size_t> (s)];
            if (fret == kMutedString)
                continue;

            const MidiNote note = fretboard.midiNoteAt (s, fret);
            sounding.insert (pitchClassOf (note));
            lowest = std::min (lowest, note);
            ++soundCount;

            if (firstString < 0)
                firstString = s;
            lastString = s;

            if (fret > 0)
            {
                lowestFret  = lowestFret == 0 ? fret : std::min (lowestFret, fret);
                highestFret = std::max (highestFret, fret);
            }
        }

        const PitchClass lowestPc = chord.bass ? *chord.bass : chord.root;
        if (soundCount == 0 || pitchClassOf (lowest) != lowestPc)
            broken.push_back (kRuleLowestNote);

        std::set<PitchClass> required { chord.root, lowestPc };
        std::set<PitchClass> allowed { chord.root, lowestPc };

        for (int interval : chord.definition.intervals)
        {
            const PitchClass pc = pitchClassOf (chord.root + interval);
            allowed.insert (pc);

            const auto& optional = chord.definition.optional;
            if (std::find (optional.begin (), optional.end (), interval) == optional.end ())
                required.insert (pc);
        }

        if (!std::includes (sounding.begin (), sounding.end (), required.begin (), required.end ()))
            broken.push_back (kRuleChordTones);

        if (highestFret - lowestFret > (difficulty == Difficulty::Beginner ? 3 : 4))
            broken.push_back (kRuleFretSpan);

        if (fingerCount (voicing) > kMaxFingers)
            broken.push_back (kRuleFingerCount);

        if (strummed && soundCount < kMinStrummedStrings)
            broken.push_back (kRuleStringCount);

        if (difficulty != Difficulty::Advanced && lastString - firstString + 1 != soundCount)
            broken.push_back (kRuleMutedString);

        if (!std::includes (allowed.begin (), allowed.end (), sounding.begin (), sounding.end ()))
            broken.push_back (kRuleOtherNote);

        return broken;
    }
}

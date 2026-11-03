#include "algo/Pitch.h"
#include "algo/Voicing.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace algo
{
    namespace
    {
        // Returns no value if a fret is below the neck or above the highest candidate fret.
        std::optional<Voicing> placeShape (const ChordShape& shape, int rootFret)
        {
            Voicing voicing;
            for (std::size_t s = 0; s < voicing.frets.size (); ++s)
            {
                if (!shape.frets[s])
                    continue;

                const int fret = rootFret + *shape.frets[s];
                if (fret < 0 || fret > kMaxCandidateFret)
                    return std::nullopt;

                voicing.frets[s] = fret;
            }
            return voicing;
        }

        bool lessByFrets (const Voicing& a, const Voicing& b)
        {
            return a.frets < b.frets;
        }

        bool sameFrets (const Voicing& a, const Voicing& b)
        {
            return a.frets == b.frets;
        }
    }

    std::vector<Voicing> shapeCandidates (const ConcreteChord& chord, const Fretboard& fretboard, const DataSet& data)
    {
        std::vector<Voicing> candidates;

        for (const auto& shape : data.shapes)
        {
            if (shape.chord != chord.definition.name)
                continue;

            const int rootString = shape.rootStringIndex ();

            if (shape.open)
            {
                const int rootFret = *shape.frets[static_cast<std::size_t> (rootString)];
                if (pitchClassOf (fretboard.midiNoteAt (rootString, rootFret)) == chord.root)
                    if (const auto voicing = placeShape (shape, 0))
                        candidates.push_back (*voicing);
                continue;
            }

            for (int rootFret = 0; rootFret <= kMaxCandidateFret; ++rootFret)
                if (pitchClassOf (fretboard.midiNoteAt (rootString, rootFret)) == chord.root)
                    if (const auto voicing = placeShape (shape, rootFret))
                        candidates.push_back (*voicing);
        }

        // A movable shape at fret 0 can be the same as an open shape.
        std::sort (candidates.begin (), candidates.end (), lessByFrets);
        candidates.erase (std::unique (candidates.begin (), candidates.end (), sameFrets), candidates.end ());
        return candidates;
    }
}

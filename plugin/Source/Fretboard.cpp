#include "Fretboard.h"

#include "algo/Pitch.h"

#include <climits>
#include <cstdlib>

VoicedChord buildTightVoicing (const algo::Fretboard& board,
                               const algo::FretPos&   rootPos,
                               const std::string&     chordName)
{
    const algo::ChordDefinition& def    = algo::bundledData ().chords.get (chordName);
    const int                    rootPC = algo::pitchClassOf (board.midiNoteAt (rootPos.string, rootPos.fret));

    VoicedChord vc;
    vc.chordName = def.name;
    vc.symbol    = algo::pitchClassName (rootPC, algo::Key {}) + def.symbol;

    // Root position is always first
    vc.positions.push_back (rootPos);

    // For every non-root tone, find the fretboard position (frets 0-12)
    // closest in fret distance to rootPos.fret
    for (int iv : def.intervals)
    {
        int           targetPC = algo::pitchClassOf (rootPC + iv);
        int           bestDist = INT_MAX;
        algo::FretPos best     = { 0, 0 };

        for (int s = 0; s < algo::kNumStrings; ++s)
        {
            for (int f = 0; f <= 12; ++f)
            {
                if (algo::pitchClassOf (board.midiNoteAt (s, f)) != targetPC) continue;

                int dist = std::abs (f - rootPos.fret);
                if (dist < bestDist ||
                    (dist == bestDist && s < best.string))
                {
                    bestDist = dist;
                    best     = { s, f };
                }
            }
        }

        vc.positions.push_back (best);
    }

    return vc;
}

#pragma once

#include "algo/Fretboard.h"

#include <string>
#include <vector>

// A voiced chord: one fretboard position per chord tone, chosen to be the
// closest fret to rootPos (tight voicing).  The first entry is always the
// root itself.
struct VoicedChord
{
    std::string                chordName; // e.g. "Major"
    std::string                symbol;    // e.g. "Gmaj7"
    std::vector<algo::FretPos> positions; // one per tone, index 0 = root
};

// Build a tight voicing: for each chord tone (pitch class), pick the single
// fretboard position (fret 0-12) whose fret is closest to rootPos.fret.
// Ties broken by lowest string index.
VoicedChord buildTightVoicing (const algo::Fretboard& board,
                               const algo::FretPos&   rootPos,
                               const std::string&     chordName);

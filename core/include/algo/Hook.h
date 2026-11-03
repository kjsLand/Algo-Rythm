#pragma once

#include "Chords.h"
#include "Contour.h"
#include "Data.h"
#include "Plan.h"
#include "Rng.h"
#include "Seeds.h"
#include "Types.h"

#include <vector>

namespace algo
{
    constexpr int  kMinHookBars        = 2;
    constexpr int  kMaxHookBars        = 4;
    constexpr int  kMinHookHits        = 5;
    constexpr int  kMaxHookHits        = 9;
    constexpr Tick kHookStepTicks      = kPpq / 4; // One sixteenth note
    constexpr int  kHookRange          = 16;       // A major 10th (REQ-13-3)
    constexpr Tick kMaxHookNote        = 720;      // A dotted quarter note (REQ-13-5)
    constexpr Tick kLastHookNote       = kPpq;
    constexpr int  kStrongHookVelocity = 90;
    constexpr int  kWeakHookVelocity   = 75;

    // The start ticks of the hook notes, from the start of the hook.
    // Each bar has 5 to 9 starts on a 16-step Euclidean pattern, rotated by 0 to 3 steps.
    std::vector<Tick> hookRhythm (int bars, const TimeSig& timeSig, Rng& rng);

    // True at beat 1 and beat 3 of a 4/4 bar, and at the two strong beats of a 6/8 bar.
    bool isStrongBeat (Tick tickInBar, const TimeSig& timeSig);

    // The index of the section that has the hook: the first chorus, or section 0 if no chorus exists.
    int hookSectionIndex (const std::vector<std::string>& roles);

    // The lowest note of the hook range. The 5th of the scale in octave 4 is the center of the range.
    MidiNote hookRangeLow (const Key& key, const DataSet& data = bundledData ());

    // The pitch classes of the chord at a tick of a section, that are also in the scale.
    std::vector<PitchClass> hookChordTones (const std::vector<Bar>& bars, Tick tickInSection, const TimeSig& timeSig, const Key& key, const DataSet& data = bundledData ());

    // The root and the 3rd of a chord, that are also in the scale. The list can be empty for a borrowed chord.
    std::vector<PitchClass> hookEndTones (const std::vector<Bar>& bars, Tick tickInSection, const TimeSig& timeSig, const Key& key, const DataSet& data = bundledData ());

    // Returns the hook notes. The start ticks are from the start of the song. Uses the stream of
    // Stage::Hook with section index 0.
    std::vector<MelodyNote> makeHook (const SongPlan& plan, const ProgressionSet& progressions, const RngFactory& rngFactory, const DataSet& data = bundledData ());
}

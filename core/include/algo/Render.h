#pragma once

#include "Data.h"
#include "Events.h"
#include "Types.h"

namespace algo
{
    constexpr double kStringGapMs    = 12.0; // The time between two strings of one stroke
    constexpr double kStringJitterMs = 4.0;  // The largest random change of a gap
    constexpr double kMinStringGapMs = 8.0;  // REQ-09-3
    constexpr double kMaxStringGapMs = 20.0; // REQ-09-3
    constexpr double kMuteMs         = 30.0;
    constexpr int    kMuteVelocity   = 40;
    constexpr int    kChuckPitch     = 39; // Hand clap
    constexpr int    kClickHighPitch = 76; // Beat 1
    constexpr int    kClickLowPitch  = 77; // The other beats
    constexpr Tick   kClickLength    = kPpq / 8;

    struct RenderOptions
    {
        bool click { true };
        bool hook { true };
        int  section { -1 }; // -1 = all sections. Otherwise only this section, from tick 0.
    };

    // The number of ticks in one bar.
    Tick barTicks (const TimeSig& timeSig);

    // The first tick of each section, and the end of the song as the last value.
    std::vector<Tick> sectionStarts (const SongStarter& starter);

    // The MIDI key signature of the key: the number of sharps (negative for flats) and the mode.
    int  keySignatureSharps (const Key& key, const DataSet& data = bundledData ());
    bool isMinorKey (const Key& key, const DataSet& data = bundledData ());

    // Velocity = 70 + 40 × accent, limited to 1 to 127.
    int strumVelocity (double accent);

    // Converts a song starter to timed events. The small random changes of the string gaps come from
    // the Render stream of each section, so the same song starter always gives the same events.
    EventList render (const SongStarter& starter, const RenderOptions& options = RenderOptions {}, const DataSet& data = bundledData ());
}

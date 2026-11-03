#pragma once

#include "Types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace algo
{
    // The tracks of a rendered song starter. Track 0 holds the tempo map.
    constexpr int kTempoTrack  = 0;
    constexpr int kGuitarTrack = 1;
    constexpr int kHookTrack   = 2;
    constexpr int kClickTrack  = 3;
    constexpr int kTrackCount  = 4;

    // MIDI channels count from 0, so channel 10 has the value 9.
    constexpr int kGuitarChannel = 0;
    constexpr int kHookChannel   = 1;
    constexpr int kDrumChannel   = 9;

    // General MIDI program numbers, counted from 0.
    constexpr int kGuitarProgram = 25; // Steel string guitar
    constexpr int kHookProgram   = 1;  // Piano

    struct NoteEvent
    {
        int  track { kGuitarTrack };
        int  channel { kGuitarChannel };
        int  pitch { 60 };
        int  velocity { 100 };
        Tick start { 0 };
        Tick length { kPpq };
    };

    enum class MetaType
    {
        Tempo         = 0x51,
        TimeSignature = 0x58,
        KeySignature  = 0x59,
        Marker        = 0x06,
        TrackName     = 0x03
    };

    // The data bytes are the bytes that follow the length in the SMF meta event.
    struct MetaEvent
    {
        int                       track { kTempoTrack };
        Tick                      tick { 0 };
        MetaType                  type { MetaType::Marker };
        std::vector<std::uint8_t> data;
    };

    struct EventList
    {
        std::vector<NoteEvent> notes;
        std::vector<MetaEvent> metas;
        Tick                   length { 0 }; // The total length in ticks
    };

    // ticks = ms × BPM × 480 / 60000, rounded to the nearest tick.
    Tick msToTicks (double ms, int bpm);

    // The inverse of msToTicks, with no rounding.
    double ticksToMs (Tick ticks, int bpm);

    MetaEvent tempoEvent (int bpm);
    MetaEvent timeSignatureEvent (const TimeSig& timeSig);

    // sharps is -7 (7 flats) to 7. minor is true for a minor key.
    MetaEvent keySignatureEvent (int sharps, bool minor);
    MetaEvent markerEvent (Tick tick, const std::string& text);
    MetaEvent trackNameEvent (int track, const std::string& text);

    // Returns the text of a marker or a track name event.
    std::string metaText (const MetaEvent& event);

    inline bool operator== (const NoteEvent& a, const NoteEvent& b)
    {
        return a.track == b.track && a.channel == b.channel && a.pitch == b.pitch && a.velocity == b.velocity && a.start == b.start && a.length == b.length;
    }

    inline bool operator== (const MetaEvent& a, const MetaEvent& b)
    {
        return a.track == b.track && a.tick == b.tick && a.type == b.type && a.data == b.data;
    }
}

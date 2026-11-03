#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace algo::test
{
    // A small Standard MIDI File reader for the tests only. It is not part of the core library.
    struct SmfEvent
    {
        std::int64_t              tick { 0 };
        std::uint8_t              status { 0 };   // 0xFF for a meta event
        std::uint8_t              metaType { 0 }; // Only for a meta event
        std::vector<std::uint8_t> data;           // The data bytes, with no status byte
    };

    struct SmfTrack
    {
        std::vector<SmfEvent> events;
    };

    struct SmfFile
    {
        int                   format { 0 };
        int                   division { 0 };
        std::vector<SmfTrack> tracks;

        // The BPM of the first tempo event, or 120 if the file has no tempo event.
        double bpm () const;

        // The number of note-on events with a velocity above 0, in all tracks.
        int noteCount () const;

        // The text of each meta event of the type, in track order.
        std::vector<std::string> metaTexts (std::uint8_t type) const;
    };

    // Throws std::runtime_error if the bytes are not a valid file.
    SmfFile readSmf (const std::vector<std::uint8_t>& bytes);
    SmfFile readSmfFile (const std::string& path);
}

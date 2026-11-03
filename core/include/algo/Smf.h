#pragma once

#include "Events.h"

#include <cstdint>
#include <filesystem>
#include <ostream>
#include <vector>

namespace algo
{
    // Writes a Standard MIDI File, type 1, at 480 PPQ. Track 0 holds the meta events of kTempoTrack.
    // The other tracks are Guitar, Hook and Click. Each note is a note-on and a note-off, with no running status.
    void writeSmf (const EventList& events, std::ostream& stream);

    std::vector<std::uint8_t> smfBytes (const EventList& events);

    // Throws std::runtime_error if the file cannot be written.
    void writeSmfFile (const EventList& events, const std::filesystem::path& path);
}

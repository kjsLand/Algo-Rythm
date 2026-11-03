#include "algo/Smf.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <tuple>

namespace algo
{
    namespace
    {
        // An event in one track, before the delta times are known.
        struct TrackEvent
        {
            Tick                      tick { 0 };
            int                       order { 0 }; // At the same tick: meta, program, note-off, note-on
            std::size_t               sequence { 0 };
            std::vector<std::uint8_t> bytes;
        };

        enum Order
        {
            kMetaOrder    = 0,
            kProgramOrder = 1,
            kNoteOffOrder = 2,
            kNoteOnOrder  = 3
        };

        void writeVarLen (std::vector<std::uint8_t>& out, std::uint32_t value)
        {
            std::uint8_t buffer[5];
            int          count = 0;
            buffer[count++]    = static_cast<std::uint8_t> (value & 0x7F);
            while ((value >>= 7) > 0)
                buffer[count++] = static_cast<std::uint8_t> ((value & 0x7F) | 0x80);
            while (count > 0)
                out.push_back (buffer[--count]);
        }

        void writeU32 (std::vector<std::uint8_t>& out, std::uint32_t value)
        {
            for (int shift = 24; shift >= 0; shift -= 8)
                out.push_back (static_cast<std::uint8_t> ((value >> shift) & 0xFF));
        }

        void writeU16 (std::vector<std::uint8_t>& out, std::uint16_t value)
        {
            out.push_back (static_cast<std::uint8_t> (value >> 8));
            out.push_back (static_cast<std::uint8_t> (value & 0xFF));
        }

        std::vector<std::uint8_t> metaBytes (const MetaEvent& meta)
        {
            std::vector<std::uint8_t> bytes { 0xFF, static_cast<std::uint8_t> (meta.type) };
            writeVarLen (bytes, static_cast<std::uint32_t> (meta.data.size ()));
            bytes.insert (bytes.end (), meta.data.begin (), meta.data.end ());
            return bytes;
        }

        std::uint8_t dataByte (int value)
        {
            return static_cast<std::uint8_t> (std::clamp (value, 0, 127));
        }

        std::uint8_t statusByte (int status, int channel)
        {
            return static_cast<std::uint8_t> (status | (channel & 0x0F));
        }

        std::vector<TrackEvent> trackEvents (const EventList& events, int track)
        {
            std::vector<TrackEvent> list;
            const auto              add = [&list] (Tick tick, int order, std::vector<std::uint8_t> bytes)
            {
                list.push_back ({ std::max<Tick> (0, tick), order, list.size (), std::move (bytes) });
            };

            for (const auto& meta : events.metas)
                if (meta.track == track)
                    add (meta.tick, kMetaOrder, metaBytes (meta));

            if (track == kGuitarTrack)
                add (0, kProgramOrder, { statusByte (0xC0, kGuitarChannel), static_cast<std::uint8_t> (kGuitarProgram) });
            if (track == kHookTrack)
                add (0, kProgramOrder, { statusByte (0xC0, kHookChannel), static_cast<std::uint8_t> (kHookProgram) });

            for (const auto& note : events.notes)
            {
                if (note.track != track)
                    continue;
                add (note.start, kNoteOnOrder, { statusByte (0x90, note.channel), dataByte (note.pitch), static_cast<std::uint8_t> (std::clamp (note.velocity, 1, 127)) });
                add (note.start + std::max<Tick> (1, note.length), kNoteOffOrder, { statusByte (0x80, note.channel), dataByte (note.pitch), 0 });
            }

            std::sort (list.begin (), list.end (), [] (const TrackEvent& a, const TrackEvent& b)
            {
                return std::tie (a.tick, a.order, a.sequence) < std::tie (b.tick, b.order, b.sequence);
            });
            return list;
        }

        std::vector<std::uint8_t> trackChunk (const EventList& events, int track)
        {
            std::vector<std::uint8_t> body;
            Tick                      now = 0;
            for (const auto& event : trackEvents (events, track))
            {
                writeVarLen (body, static_cast<std::uint32_t> (event.tick - now));
                body.insert (body.end (), event.bytes.begin (), event.bytes.end ());
                now = event.tick;
            }

            // The end of the track is at the end of the song, so a DAW makes a clip with the correct length.
            writeVarLen (body, static_cast<std::uint32_t> (std::max<Tick> (0, events.length - now)));
            body.insert (body.end (), { 0xFF, 0x2F, 0x00 });

            std::vector<std::uint8_t> chunk { 'M', 'T', 'r', 'k' };
            writeU32 (chunk, static_cast<std::uint32_t> (body.size ()));
            chunk.insert (chunk.end (), body.begin (), body.end ());
            return chunk;
        }
    }

    std::vector<std::uint8_t> smfBytes (const EventList& events)
    {
        std::vector<std::uint8_t> out { 'M', 'T', 'h', 'd' };
        writeU32 (out, 6);
        writeU16 (out, 1); // Format 1
        writeU16 (out, static_cast<std::uint16_t> (kTrackCount));
        writeU16 (out, static_cast<std::uint16_t> (kPpq));

        for (int track = 0; track < kTrackCount; ++track)
        {
            const auto chunk = trackChunk (events, track);
            out.insert (out.end (), chunk.begin (), chunk.end ());
        }
        return out;
    }

    void writeSmf (const EventList& events, std::ostream& stream)
    {
        const auto bytes = smfBytes (events);
        stream.write (reinterpret_cast<const char*> (bytes.data ()), static_cast<std::streamsize> (bytes.size ()));
    }

    void writeSmfFile (const EventList& events, const std::filesystem::path& path)
    {
        std::ofstream stream (path, std::ios::binary);
        if (!stream)
            throw std::runtime_error ("The MIDI file cannot be written: " + path.string ());
        writeSmf (events, stream);
        if (!stream)
            throw std::runtime_error ("The MIDI file cannot be written: " + path.string ());
    }
}

#include "SmfReader.h"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace algo::test
{
    namespace
    {
        class Cursor
        {
        public:
            Cursor (const std::vector<std::uint8_t>& bytes, std::size_t begin, std::size_t end)
                : m_bytes (bytes),
                  m_pos (begin),
                  m_end (end)
            {
            }

            bool done () const
            {
                return m_pos >= m_end;
            }

            std::uint8_t byte ()
            {
                if (m_pos >= m_end)
                    throw std::runtime_error ("The MIDI data ends too soon");
                return m_bytes[m_pos++];
            }

            std::uint32_t u32 ()
            {
                std::uint32_t value = 0;
                for (int i = 0; i < 4; ++i)
                    value = (value << 8) | byte ();
                return value;
            }

            std::uint16_t u16 ()
            {
                const auto hi = byte ();
                return static_cast<std::uint16_t> ((hi << 8) | byte ());
            }

            std::uint32_t varLen ()
            {
                std::uint32_t value = 0;
                for (int i = 0; i < 4; ++i)
                {
                    const auto b = byte ();
                    value        = (value << 7) | (b & 0x7F);
                    if ((b & 0x80) == 0)
                        return value;
                }
                throw std::runtime_error ("A variable-length value has more than 4 bytes");
            }

            std::size_t pos () const
            {
                return m_pos;
            }

        private:
            const std::vector<std::uint8_t>& m_bytes;
            std::size_t                      m_pos;
            std::size_t                      m_end;
        };

        std::string chunkId (Cursor& cursor)
        {
            std::string id;
            for (int i = 0; i < 4; ++i)
                id += static_cast<char> (cursor.byte ());
            return id;
        }

        int dataBytes (std::uint8_t status)
        {
            switch (status & 0xF0)
            {
                case 0xC0:
                case 0xD0: return 1;
                default: return 2;
            }
        }
    }

    double SmfFile::bpm () const
    {
        for (const auto& track : tracks)
            for (const auto& event : track.events)
                if (event.status == 0xFF && event.metaType == 0x51 && event.data.size () == 3)
                {
                    const double microseconds = (event.data[0] << 16) | (event.data[1] << 8) | event.data[2];
                    return 60000000.0 / microseconds;
                }
        return 120.0;
    }

    int SmfFile::noteCount () const
    {
        int count = 0;
        for (const auto& track : tracks)
            for (const auto& event : track.events)
                if ((event.status & 0xF0) == 0x90 && event.data.size () == 2 && event.data[1] > 0)
                    ++count;
        return count;
    }

    std::vector<std::string> SmfFile::metaTexts (std::uint8_t type) const
    {
        std::vector<std::string> texts;
        for (const auto& track : tracks)
            for (const auto& event : track.events)
                if (event.status == 0xFF && event.metaType == type)
                    texts.emplace_back (event.data.begin (), event.data.end ());
        return texts;
    }

    SmfFile readSmf (const std::vector<std::uint8_t>& bytes)
    {
        Cursor header (bytes, 0, bytes.size ());
        if (chunkId (header) != "MThd" || header.u32 () != 6)
            throw std::runtime_error ("The file has no MThd header");

        SmfFile file;
        file.format          = header.u16 ();
        const int trackCount = header.u16 ();
        file.division        = header.u16 ();

        std::size_t pos = header.pos ();
        for (int t = 0; t < trackCount; ++t)
        {
            Cursor chunk (bytes, pos, bytes.size ());
            if (chunkId (chunk) != "MTrk")
                throw std::runtime_error ("Track " + std::to_string (t) + " has no MTrk header");
            const auto length = chunk.u32 ();
            const auto begin  = chunk.pos ();
            if (begin + length > bytes.size ())
                throw std::runtime_error ("Track " + std::to_string (t) + " is longer than the file");

            Cursor       cursor (bytes, begin, begin + length);
            SmfTrack     track;
            std::int64_t tick    = 0;
            std::uint8_t running = 0;
            bool         ended   = false;
            while (!cursor.done ())
            {
                tick += cursor.varLen ();
                SmfEvent event;
                event.tick = tick;

                std::uint8_t status = cursor.byte ();
                if (status < 0x80)
                {
                    if (running == 0)
                        throw std::runtime_error ("A data byte has no status byte");
                    event.data.push_back (status);
                    status = running;
                }
                event.status = status;

                if (status == 0xFF)
                {
                    event.metaType  = cursor.byte ();
                    const auto size = cursor.varLen ();
                    for (std::uint32_t i = 0; i < size; ++i)
                        event.data.push_back (cursor.byte ());
                    if (event.metaType == 0x2F)
                        ended = true;
                }
                else if (status == 0xF0 || status == 0xF7)
                {
                    const auto size = cursor.varLen ();
                    for (std::uint32_t i = 0; i < size; ++i)
                        event.data.push_back (cursor.byte ());
                }
                else
                {
                    running = status;
                    while (static_cast<int> (event.data.size ()) < dataBytes (status))
                        event.data.push_back (cursor.byte ());
                }
                track.events.push_back (event);
            }
            if (!ended)
                throw std::runtime_error ("Track " + std::to_string (t) + " has no end-of-track event");

            file.tracks.push_back (track);
            pos = begin + length;
        }
        return file;
    }

    SmfFile readSmfFile (const std::string& path)
    {
        std::ifstream stream (path, std::ios::binary);
        if (!stream)
            throw std::runtime_error ("The file cannot be read: " + path);
        return readSmf (std::vector<std::uint8_t> (std::istreambuf_iterator<char> (stream), {}));
    }
}

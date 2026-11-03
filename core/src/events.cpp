#include "algo/Events.h"

#include <cmath>
#include <stdexcept>

namespace algo
{
    namespace
    {
        MetaEvent textEvent (int track, Tick tick, MetaType type, const std::string& text)
        {
            return { track, tick, type, std::vector<std::uint8_t> (text.begin (), text.end ()) };
        }
    }

    Tick msToTicks (double ms, int bpm)
    {
        return static_cast<Tick> (std::llround (ms * bpm * kPpq / 60000.0));
    }

    double ticksToMs (Tick ticks, int bpm)
    {
        if (bpm <= 0)
            throw std::invalid_argument ("The BPM must be positive");
        return static_cast<double> (ticks) * 60000.0 / (static_cast<double> (bpm) * kPpq);
    }

    MetaEvent tempoEvent (int bpm)
    {
        if (bpm <= 0)
            throw std::invalid_argument ("The BPM must be positive");

        const auto microseconds = static_cast<std::uint32_t> (std::llround (60000000.0 / bpm));
        return { kTempoTrack,
                 0,
                 MetaType::Tempo,
                 { static_cast<std::uint8_t> ((microseconds >> 16) & 0xFF), static_cast<std::uint8_t> ((microseconds >> 8) & 0xFF), static_cast<std::uint8_t> (microseconds & 0xFF) } };
    }

    MetaEvent timeSignatureEvent (const TimeSig& timeSig)
    {
        std::uint8_t power = 0;
        while ((1 << power) < timeSig.denominator)
            ++power;

        // A 4/4 click is on each quarter note (24 MIDI clocks). A 6/8 click is on each dotted quarter note (36).
        const std::uint8_t clocks = timeSig.denominator == 8 && timeSig.numerator % 3 == 0 ? 36 : static_cast<std::uint8_t> (96 / timeSig.denominator);
        return { kTempoTrack, 0, MetaType::TimeSignature, { static_cast<std::uint8_t> (timeSig.numerator), power, clocks, 8 } };
    }

    MetaEvent keySignatureEvent (int sharps, bool minor)
    {
        if (sharps < -7 || sharps > 7)
            throw std::invalid_argument ("A key signature has -7 to 7 sharps: " + std::to_string (sharps));
        return { kTempoTrack, 0, MetaType::KeySignature, { static_cast<std::uint8_t> (static_cast<std::int8_t> (sharps)), static_cast<std::uint8_t> (minor ? 1 : 0) } };
    }

    MetaEvent markerEvent (Tick tick, const std::string& text)
    {
        return textEvent (kTempoTrack, tick, MetaType::Marker, text);
    }

    MetaEvent trackNameEvent (int track, const std::string& text)
    {
        return textEvent (track, 0, MetaType::TrackName, text);
    }

    std::string metaText (const MetaEvent& event)
    {
        return std::string (event.data.begin (), event.data.end ());
    }
}

#include "algo/Engine.h"
#include "algo/Render.h"
#include "algo/Smf.h"

#include "support/Golden.h"
#include "support/SmfReader.h"
#include "support/StarterHelpers.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <sstream>

using namespace algo;

namespace
{
    std::string asText (const std::vector<std::uint8_t>& bytes)
    {
        return std::string (bytes.begin (), bytes.end ());
    }
}

TEST_CASE ("The header is MThd, format 1, 4 tracks and division 480")
{
    EventList events;
    events.metas.push_back (tempoEvent (120));
    const auto bytes = smfBytes (events);

    const std::vector<std::uint8_t> header { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 4, 0x01, 0xE0 };
    REQUIRE (std::vector<std::uint8_t> (bytes.begin (), bytes.begin () + 14) == header);
}

TEST_CASE ("Delta times are variable-length values and each track ends with FF 2F 00")
{
    EventList events;
    events.length = 0x4000;
    events.notes.push_back ({ kClickTrack, kDrumChannel, 76, 100, 0x80, 0x3F80 });

    const auto file = algo::test::readSmf (smfBytes (events));
    REQUIRE (file.tracks.size () == 4);
    const auto& click = file.tracks[3].events;
    REQUIRE (click.size () == 3);
    REQUIRE (click[0].tick == 0x80);
    REQUIRE (click[0].status == 0x99);
    REQUIRE (click[1].tick == 0x4000);
    REQUIRE (click[1].status == 0x89);
    REQUIRE (click[2].metaType == 0x2F);

    // 0x80 is written as 81 00.
    const auto bytes = smfBytes (events);
    const auto text  = asText (bytes);
    REQUIRE (text.find (std::string ("\x81\x00\x99", 3)) != std::string::npos);
}

TEST_CASE ("Note-off events come before note-on events at the same tick, with no running status")
{
    EventList events;
    events.length = 960;
    events.notes.push_back ({ kGuitarTrack, kGuitarChannel, 60, 100, 0, 480 });
    events.notes.push_back ({ kGuitarTrack, kGuitarChannel, 60, 100, 480, 480 });

    const auto  file   = algo::test::readSmf (smfBytes (events));
    const auto& guitar = file.tracks[1].events;
    // Program change, on, off, on, off, end of track
    REQUIRE (guitar.size () == 6);
    REQUIRE (guitar[0].status == 0xC0);
    REQUIRE (guitar[0].data[0] == kGuitarProgram);
    REQUIRE (guitar[2].status == 0x80);
    REQUIRE (guitar[3].status == 0x90);
    REQUIRE (guitar[2].tick == guitar[3].tick);

    // With no running status, each channel event has its own status byte.
    const auto bytes = smfBytes (events);
    const auto count = std::count (bytes.begin (), bytes.end (), static_cast<std::uint8_t> (0x90));
    REQUIRE (count == 2);
}

TEST_CASE ("The tempo, time signature and key signature are at tick 0 of track 0")
{
    GenerationSettings settings;
    settings.masterSeed = 3;
    const auto starter  = generate (settings);
    const auto file     = algo::test::readSmf (smfBytes (render (starter)));

    REQUIRE (file.format == 1);
    REQUIRE (file.division == 480);
    REQUIRE (file.bpm () == Catch::Approx (starter.bpm).margin (0.01));
    const auto& tempo = file.tracks[0].events;
    REQUIRE (tempo[0].metaType == 0x51);
    REQUIRE (tempo[1].metaType == 0x58);
    REQUIRE (tempo[2].metaType == 0x59);
    REQUIRE (file.metaTexts (0x03) == std::vector<std::string> { "Guitar", "Hook", "Click" });
}

TEST_CASE ("The MIDI files of the golden song starters match the stored files byte for byte")
{
    for (const auto& [name, settings] : algo::test::goldenSettings ())
    {
        CAPTURE (name);
        const auto starter = generate (settings);
        algo::test::requireGolden ("smf/" + name + ".mid", asText (smfBytes (render (starter))));
    }
}

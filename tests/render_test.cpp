#include "algo/Engine.h"
#include "algo/Render.h"

#include "support/PropertyTest.h"
#include "support/StarterHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <map>

using namespace algo;

namespace
{
    // One section of 8 bars of G major (320003) with one Down stroke on each beat.
    SongStarter quarterDowns (int bpm = 120)
    {
        SongStarter starter;
        starter.bpm = bpm;
        starter.key = { 7, "Major" };

        Section section;
        section.name         = "Verse";
        section.role         = "verse";
        section.strum.steps  = 8;
        section.strum.slots  = { Stroke::Down, Stroke::Rest, Stroke::Down, Stroke::Rest, Stroke::Down, Stroke::Rest, Stroke::Down, Stroke::Rest };
        section.strum.accent = { 0.6, 0.0, 1.0, 0.0, 0.6, 0.0, 1.0, 0.0 };

        Bar bar;
        bar.chords.push_back (ChordSymbol {});
        Voicing g;
        g.frets = { 3, 2, 0, 0, 0, 3 };
        bar.voicings.push_back (g);
        section.bars.assign (8, bar);

        starter.sections.push_back (section);
        return starter;
    }

    std::vector<NoteEvent> notesOf (const EventList& events, int track, int channel)
    {
        std::vector<NoteEvent> notes;
        for (const auto& note : events.notes)
            if (note.track == track && note.channel == channel)
                notes.push_back (note);
        return notes;
    }
}

TEST_CASE ("msToTicks uses ticks = ms x BPM x 480 / 60000 and rounds")
{
    REQUIRE (msToTicks (15, 120) == 14);
    REQUIRE (msToTicks (500, 120) == 480);
    REQUIRE (msToTicks (0, 90) == 0);
    REQUIRE (ticksToMs (480, 120) == 500.0);
}

TEST_CASE ("A Down stroke on a 6-string voicing gives 6 notes from low to high")
{
    const auto events = render (quarterDowns (), RenderOptions { false, false, -1 });
    const auto guitar = notesOf (events, kGuitarTrack, kGuitarChannel);
    REQUIRE (guitar.size () == 8 * 4 * 6);

    for (std::size_t i = 1; i < 6; ++i)
    {
        REQUIRE (guitar[i].pitch > guitar[i - 1].pitch);
        REQUIRE (guitar[i].start > guitar[i - 1].start);
    }
    REQUIRE (guitar[0].pitch == 43); // G2
    REQUIRE (guitar[0].start == 0);

    // Each string of the G chord has its own pitch. A note sounds until the next stroke on its string,
    // or the end of the bar (REQ-09-4).
    std::map<int, std::vector<NoteEvent>> byPitch;
    for (const auto& note : guitar)
        byPitch[note.pitch].push_back (note);
    REQUIRE (byPitch.size () == 6);
    for (const auto& [pitch, notes] : byPitch)
    {
        CAPTURE (pitch);
        for (std::size_t i = 0; i < notes.size (); ++i)
        {
            const Tick end    = notes[i].start + notes[i].length;
            const Tick barEnd = (notes[i].start / 1920 + 1) * 1920;
            if (i + 1 < notes.size () && notes[i + 1].start < barEnd)
                REQUIRE (end == notes[i + 1].start);
            else
                REQUIRE (end == barEnd);
        }
    }
}

TEST_CASE ("An Up stroke sounds its strings from high to low (REQ-09-2)")
{
    auto starter                      = quarterDowns ();
    starter.sections[0].strum.slots   = { Stroke::Down, Stroke::Up, Stroke::Rest, Stroke::Rest, Stroke::Rest, Stroke::Rest, Stroke::Rest, Stroke::Rest };
    starter.sections[0].strum.strings = 4;
    starter.sections[0].bars.resize (1);

    const auto guitar = notesOf (render (starter, RenderOptions { false, false, -1 }), kGuitarTrack, kGuitarChannel);
    REQUIRE (guitar.size () == 8); // The top 4 strings for each stroke
    for (std::size_t i = 5; i < 8; ++i)
    {
        REQUIRE (guitar[i].pitch < guitar[i - 1].pitch);
        REQUIRE (guitar[i].start > guitar[i - 1].start);
    }
    REQUIRE (guitar[4].pitch == 67); // The high E string starts the Up stroke
}

TEST_CASE ("The velocity is 70 + 40 x accent, from 1 to 127")
{
    REQUIRE (strumVelocity (0.0) == 70);
    REQUIRE (strumVelocity (0.6) == 94);
    REQUIRE (strumVelocity (1.0) == 110);
    REQUIRE (strumVelocity (5.0) == 127);
    REQUIRE (strumVelocity (-5.0) == 1);
}

TEST_CASE ("The strings of a stroke are 8 to 20 ms apart and each note sounds until the next stroke (REQ-09-3, REQ-09-4)")
{
    const std::uint64_t count = std::min<std::uint64_t> (algo::test::propertySeedCount (), 1000);
    for (std::uint64_t seed = 1; seed <= count; ++seed)
    {
        CAPTURE (seed);
        for (const auto difficulty : { Difficulty::Beginner, Difficulty::Advanced })
        {
            const auto starter = generate (algo::test::settingsFor (seed, difficulty));
            const auto events  = render (starter, RenderOptions { false, false, -1 });
            const Tick bar     = barTicks (starter.timeSig);

            // Group the notes of each stroke by their slot. The last string of a stroke starts before the next slot.
            const auto starts = sectionStarts (starter);
            const auto guitar = notesOf (events, kGuitarTrack, kGuitarChannel);
            for (std::size_t i = 0; i < starter.sections.size (); ++i)
            {
                const Tick                        slot = bar / std::max (1, starter.sections[i].strum.steps);
                std::map<Tick, std::vector<Tick>> strokes;
                for (const auto& note : guitar)
                    if (note.start >= starts[i] && note.start < starts[i + 1])
                        strokes[(note.start - starts[i]) / slot].push_back (note.start);

                for (auto& [index, onsets] : strokes)
                {
                    std::sort (onsets.begin (), onsets.end ());
                    REQUIRE (onsets.front () == starts[i] + index * slot);
                    for (std::size_t k = 1; k < onsets.size (); ++k)
                    {
                        const double ms = ticksToMs (onsets[k] - onsets[k - 1], starter.bpm);
                        REQUIRE (ms >= kMinStringGapMs);
                        REQUIRE (ms <= kMaxStringGapMs);
                    }
                }
            }

            // A strummed note ends at the next note on its string, or at the end of the bar.
            for (const auto& note : guitar)
            {
                REQUIRE (note.length > 0);
                const Tick end = note.start + note.length;
                REQUIRE (end <= (note.start / bar + 1) * bar + bar / 4);
            }
        }
    }
}

TEST_CASE ("A Mute is 30 ms at velocity 40 and a Chuck is one hand clap on channel 10")
{
    auto starter                    = quarterDowns ();
    starter.sections[0].strum.slots = { Stroke::Down, Stroke::Mute, Stroke::Chuck, Stroke::Rest, Stroke::Down, Stroke::Rest, Stroke::Rest, Stroke::Rest };
    starter.sections[0].bars.resize (1);

    const auto events = render (starter, RenderOptions { false, false, -1 });
    const auto guitar = notesOf (events, kGuitarTrack, kGuitarChannel);
    const auto drums  = notesOf (events, kGuitarTrack, kDrumChannel);

    REQUIRE (drums.size () == 1);
    REQUIRE (drums[0].pitch == kChuckPitch);
    REQUIRE (drums[0].start == 480);

    const auto mutes = std::count_if (guitar.begin (), guitar.end (), [] (const NoteEvent& n)
    {
        return n.velocity == kMuteVelocity;
    });
    REQUIRE (mutes == 6);
    for (const auto& note : guitar)
        if (note.velocity == kMuteVelocity)
            REQUIRE (note.length == msToTicks (kMuteMs, starter.bpm));
}

TEST_CASE ("An 8-bar 4/4 section gives 32 click notes")
{
    const auto events = render (quarterDowns ());
    const auto click  = notesOf (events, kClickTrack, kDrumChannel);
    REQUIRE (click.size () == 32);
    REQUIRE (click[0].pitch == kClickHighPitch);
    REQUIRE (click[1].pitch == kClickLowPitch);
    REQUIRE (click[4].pitch == kClickHighPitch);
    REQUIRE (click[4].start == 1920);
}

TEST_CASE ("Each section has a marker at its first tick, and the tracks have names")
{
    GenerationSettings settings;
    settings.form       = "Verse-Chorus-Bridge";
    settings.masterSeed = 5;
    const auto starter  = generate (settings);
    const auto events   = render (starter);
    const auto starts   = sectionStarts (starter);

    std::vector<std::pair<Tick, std::string>> markers;
    std::vector<std::string>                  names;
    for (const auto& meta : events.metas)
    {
        if (meta.type == MetaType::Marker)
            markers.emplace_back (meta.tick, metaText (meta));
        if (meta.type == MetaType::TrackName)
            names.push_back (metaText (meta));
    }

    REQUIRE (markers.size () == 3);
    for (std::size_t i = 0; i < markers.size (); ++i)
    {
        REQUIRE (markers[i].first == starts[i]);
        REQUIRE (markers[i].second == starter.sections[i].name);
    }
    REQUIRE (names == std::vector<std::string> { "Guitar", "Hook", "Click" });
    REQUIRE (events.length == starts.back ());
    REQUIRE (events.metas[0].type == MetaType::Tempo);
    REQUIRE (events.metas[1].type == MetaType::TimeSignature);
    REQUIRE (events.metas[2].type == MetaType::KeySignature);
}

TEST_CASE ("The key signature follows the parent major scale")
{
    REQUIRE (keySignatureSharps ({ 0, "Major" }) == 0);
    REQUIRE (keySignatureSharps ({ 7, "Major" }) == 1);
    REQUIRE (keySignatureSharps ({ 5, "Major" }) == -1);
    REQUIRE (keySignatureSharps ({ 9, "Natural Minor" }) == 0);
    REQUIRE (keySignatureSharps ({ 2, "Dorian" }) == 0);
    REQUIRE (keySignatureSharps ({ 6, "Major" }) == -6); // Gb major
    REQUIRE (keySignatureSharps ({ 3, "Major" }) == -3);
    REQUIRE (isMinorKey ({ 9, "Natural Minor" }));
    REQUIRE_FALSE (isMinorKey ({ 9, "Major" }));
}

TEST_CASE ("A section render starts at tick 0 and has the length of the section")
{
    GenerationSettings settings;
    settings.masterSeed = 11;
    const auto starter  = generate (settings);
    const auto events   = render (starter, RenderOptions { true, true, 1 });
    const Tick length   = barTicks (starter.timeSig) * static_cast<Tick> (starter.sections[1].bars.size ());

    REQUIRE (events.length == length);
    for (const auto& note : events.notes)
    {
        REQUIRE (note.start >= 0);
        REQUIRE (note.start < length);
    }
}

TEST_CASE ("The same song starter always renders the same events")
{
    const auto starter = generate (algo::test::settingsFor (77, Difficulty::Advanced, true));
    const auto a       = render (starter);
    const auto b       = render (starter);
    REQUIRE (a.notes == b.notes);
    REQUIRE (a.metas == b.metas);
}

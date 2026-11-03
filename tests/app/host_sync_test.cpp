#include "Sequencer.h"

#include "algo/Engine.h"
#include "algo/Render.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>

// The plugin reads the host tempo and PPQ position and sends the events of each block (REQ-16-2).
TEST_CASE ("Host sync: the events follow the host tempo and position")
{
    const double bpm        = GENERATE (70.0, 120.0, 170.0);
    const double sampleRate = 48000.0;
    const int    block      = 512;
    CAPTURE (bpm);

    algo::GenerationSettings settings;
    settings.masterSeed   = 5;
    const auto   starter  = algo::generate (settings);
    const auto   schedule = EventSchedule::fromStarter (starter);
    const double spt      = sampleRate * 60.0 / (bpm * algo::kPpq);

    SoundingNotes    sounding {};
    juce::MidiBuffer midi;
    midi.ensureSize (65536);

    // Play 2 passes of the song from a host position of 3.5 quarter notes, as a host does.
    const double startPpq   = 3.5;
    const double totalTicks = 2.0 * static_cast<double> (schedule->length);
    long long    sample     = 0;
    int          noteOns    = 0;
    int          checked    = 0;
    for (; sample / spt < totalTicks; sample += block)
    {
        const double ppq       = startPpq + sample / spt / algo::kPpq;
        const double startTick = ppq * algo::kPpq;
        const double endTick   = startTick + block / spt;
        midi.clear ();
        collectSongEvents (*schedule, startTick, endTick, spt, block, midi, sounding);

        for (const auto metadata : midi)
        {
            if (!metadata.getMessage ().isNoteOn ())
                continue;
            ++noteOns;

            // The event tick in the song, from the sample position of the event.
            const double hostTick = startTick + metadata.samplePosition / spt;
            const double songTick = std::fmod (hostTick, static_cast<double> (schedule->length));
            const auto   index    = schedule->firstAtOrAfter (songTick - 1.0 / spt * 1.5);
            REQUIRE (index < schedule->events.size ());
            REQUIRE (std::abs (static_cast<double> (schedule->events[index].tick) - songTick) * (1000.0 * 60.0 / (bpm * algo::kPpq)) <= 1.0);
            ++checked;
        }
    }

    int onsInSchedule = 0;
    for (const auto& e : schedule->events)
        onsInSchedule += e.on ? 1 : 0;

    // About 2 passes: the first pass starts at 3.5 quarter notes.
    REQUIRE (noteOns >= 2 * onsInSchedule - 200);
    REQUIRE (noteOns <= 2 * onsInSchedule + 200);
    REQUIRE (checked == noteOns);
}

TEST_CASE ("A note-off is sent only for a note that sounds")
{
    const auto       schedule = EventSchedule::fromStarter (algo::generate (algo::GenerationSettings {}));
    SoundingNotes    sounding {};
    juce::MidiBuffer midi;
    midi.ensureSize (65536);

    // Start in the middle of the song: the first note-offs of notes that started before are not sent.
    collectSongEvents (*schedule, 1000.0, 3000.0, 10.0, 20000, midi, sounding);
    int on = 0, off = 0;
    for (const auto metadata : midi)
    {
        on += metadata.getMessage ().isNoteOn () ? 1 : 0;
        off += metadata.getMessage ().isNoteOff () ? 1 : 0;
    }
    REQUIRE (on > 0);
    REQUIRE (off <= on);
}

TEST_CASE ("The plugin MIDI output can leave out the click track")
{
    const auto       schedule = EventSchedule::fromStarter (algo::generate (algo::GenerationSettings {}));
    SoundingNotes    sounding {};
    juce::MidiBuffer midi;
    midi.ensureSize (65536);
    collectSongEvents (*schedule, 0.0, 3840.0, 10.0, 40000, midi, sounding, false);
    for (const auto metadata : midi)
        if (metadata.getMessage ().isNoteOn () && metadata.getMessage ().getChannel () == 10)
            REQUIRE (metadata.getMessage ().getNoteNumber () < 76); // Only a chuck (39), no click (76 or 77)
}

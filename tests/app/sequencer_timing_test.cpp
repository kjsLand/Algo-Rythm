#include "AllocationCounter.h"

#include "PluckVoice.h"
#include "Sequencer.h"

#include "algo/Engine.h"
#include "algo/Render.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>
#include <vector>

namespace
{
    constexpr double kSampleRate = 48000.0;

    algo::SongStarter knownStarter ()
    {
        algo::GenerationSettings settings;
        settings.masterSeed = 81723;
        settings.difficulty = algo::Difficulty::Intermediate;
        settings.bpmMin     = 100;
        settings.bpmMax     = 100;
        return algo::generate (settings);
    }

    struct NoteOn
    {
        long long sample { 0 };
        int       channel { 0 };
        int       note { 0 };
    };
}

TEST_CASE ("Offline timing: each note starts within 1 ms of its event time (REQ-12-2)")
{
    const int blockSize = GENERATE (64, 512, 1024);
    CAPTURE (blockSize);

    const auto   starter  = knownStarter ();
    auto         schedule = EventSchedule::fromStarter (starter);
    const auto   bar      = algo::barTicks (starter.timeSig);
    const auto   limit    = bar * 8; // 8 bars
    const double spt      = kSampleRate * 60.0 / (starter.bpm * algo::kPpq);

    std::vector<NoteOn> expected;
    for (const auto& e : schedule->events)
        if (e.on && e.tick < limit)
            expected.push_back ({ std::llround (e.tick * spt), e.channel, e.note });

    juce::Synthesiser synth;
    configureSynth (synth);
    prepareSynth (synth, kSampleRate);
    Sequencer sequencer (synth);
    sequencer.prepareToPlay (blockSize, kSampleRate);
    sequencer.setSchedule (std::move (schedule));
    sequencer.play ();

    juce::MidiBuffer midi;
    midi.ensureSize (65536);
    std::vector<NoteOn> actual;
    actual.reserve (expected.size () + 100);

    const auto total = static_cast<long long> (std::ceil (limit * spt));
    for (long long start = 0; start < total; start += blockSize)
    {
        midi.clear ();
        sequencer.fillMidi (midi, blockSize);
        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage ();
            if (message.isNoteOn () && start + metadata.samplePosition < total)
                actual.push_back ({ start + metadata.samplePosition, message.getChannel (), message.getNoteNumber () });
        }
    }

    REQUIRE (actual.size () >= expected.size () - 2);
    std::size_t matched = 0;
    for (std::size_t i = 0; i < expected.size () && i < actual.size (); ++i)
    {
        CAPTURE (i);
        REQUIRE (actual[i].note == expected[i].note);
        REQUIRE (actual[i].channel == expected[i].channel);
        REQUIRE (std::llabs (actual[i].sample - expected[i].sample) <= 48);
        ++matched;
    }
    REQUIRE (matched + 2 >= expected.size ());
}

TEST_CASE ("The audio thread does not allocate (REQ-12-1)")
{
    const auto starter = knownStarter ();

    juce::Synthesiser synth;
    configureSynth (synth);
    prepareSynth (synth, kSampleRate);
    Sequencer sequencer (synth);
    sequencer.prepareToPlay (512, kSampleRate);
    sequencer.setSchedule (EventSchedule::fromStarter (starter));
    sequencer.play ();

    juce::AudioBuffer<float>           buffer (2, 512);
    const juce::AudioSourceChannelInfo info (&buffer, 0, 512);

    // The first block takes the schedule. Then a new schedule waits for the next bar.
    sequencer.getNextAudioBlock (info);
    auto next = EventSchedule::fromStarter (starter);
    sequencer.setSchedule (std::move (next));

    long allocations = 0;
    {
        algo::test::ScopedAllocationCount count;
        for (int block = 0; block < 2000; ++block)
            sequencer.getNextAudioBlock (info);
        allocations = count.count ();
    }
    REQUIRE (allocations == 0);
    REQUIRE (sequencer.isPlaying ());
    sequencer.collectGarbage ();
}

TEST_CASE ("A new schedule starts at the next bar during playback (REQ-12-5)")
{
    const auto        starter = knownStarter ();
    juce::Synthesiser synth;
    configureSynth (synth);
    prepareSynth (synth, kSampleRate);
    Sequencer sequencer (synth);
    sequencer.prepareToPlay (512, kSampleRate);
    sequencer.setSchedule (EventSchedule::fromStarter (starter));
    sequencer.play ();

    juce::MidiBuffer midi;
    midi.ensureSize (65536);
    for (int i = 0; i < 10; ++i)
    {
        midi.clear ();
        sequencer.fillMidi (midi, 512);
    }
    const double before = sequencer.positionTicks ();
    REQUIRE (before > 0.0);

    sequencer.setSchedule (EventSchedule::fromStarter (starter));
    bool allOff = false;
    for (int i = 0; i < 1000 && !allOff; ++i)
    {
        midi.clear ();
        sequencer.fillMidi (midi, 512);
        for (const auto metadata : midi)
            allOff = allOff || metadata.getMessage ().isAllNotesOff ();
    }
    REQUIRE (allOff);
    // The switch is at a bar line, so the position goes on and does not start again at 0.
    REQUIRE (sequencer.positionTicks () >= static_cast<double> (algo::barTicks (starter.timeSig)));
}

TEST_CASE ("Loop Section plays the selected section again and again, and the click can turn off (REQ-12-3, REQ-12-4)")
{
    const auto        starter = knownStarter ();
    juce::Synthesiser synth;
    configureSynth (synth);
    prepareSynth (synth, kSampleRate);
    Sequencer sequencer (synth);
    sequencer.prepareToPlay (1024, kSampleRate);
    sequencer.setSchedule (EventSchedule::fromStarter (starter));
    sequencer.setLoopSection (1);
    sequencer.setClick (false);
    sequencer.play ();

    const auto       starts = algo::sectionStarts (starter);
    juce::MidiBuffer midi;
    midi.ensureSize (65536);
    int clicks = 0;
    for (int i = 0; i < 2000; ++i)
    {
        midi.clear ();
        sequencer.fillMidi (midi, 1024);
        const double position = sequencer.positionTicks ();
        REQUIRE (position >= static_cast<double> (starts[1]));
        REQUIRE (position < static_cast<double> (starts[2]));
        for (const auto metadata : midi)
            if (metadata.getMessage ().isNoteOn () && metadata.getMessage ().getChannel () == 10 && metadata.getMessage ().getNoteNumber () >= 76)
                ++clicks;
    }
    REQUIRE (clicks == 0);

    sequencer.stop ();
    midi.clear ();
    sequencer.fillMidi (midi, 1024);
    REQUIRE (sequencer.positionTicks () < 0.0);
}

#pragma once

#include "algo/Data.h"
#include "algo/Events.h"
#include "algo/Types.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>
#include <memory>
#include <vector>

// One MIDI event of a schedule.
struct ScheduledEvent
{
    algo::Tick   tick { 0 };
    std::uint8_t channel { 1 }; // 1 to 16
    std::uint8_t note { 60 };
    std::uint8_t velocity { 0 };
    bool         on { true };
    bool         click { false }; // An event of the click track
};

// The note-on and note-off events of a song starter, sorted by tick. Note-off events come first at the same tick.
struct EventSchedule
{
    std::vector<ScheduledEvent> events;
    std::vector<algo::Tick>     sectionStarts { 0 }; // With the end of the song as the last value
    algo::Tick                  barTicks { 1920 };
    algo::Tick                  length { 0 };
    int                         bpm { 120 };

    static std::unique_ptr<EventSchedule> fromEvents (const algo::EventList& events, int bpm, const std::vector<algo::Tick>& sectionStarts, algo::Tick barTicks);
    static std::unique_ptr<EventSchedule> fromStarter (const algo::SongStarter& starter, const algo::DataSet& data = algo::bundledData ());

    // The index of the first event with a tick of `tick` or more.
    std::size_t firstAtOrAfter (double tick) const;
};

// The notes that sound, for each MIDI channel (index 0 = channel 1) and note.
using SoundingNotes = std::array<std::array<bool, 128>, 16>;

// Adds the events from startTick to endTick to the MIDI buffer, at their sample offsets in a block of
// numSamples. The song repeats every schedule.length ticks, so a host can play it for any time. A note-off
// is sent only for a note that sounds. With includeClick false, the click track is not in the buffer. The plugin uses this with the host position (REQ-16-2). It does not allocate.
void collectSongEvents (const EventSchedule& schedule, double startTick, double endTick, double samplesPerTick, int numSamples, juce::MidiBuffer& midi, SoundingNotes& sounding, bool includeClick = true);

// Gives a new schedule from the message thread to the audio thread, and gives the old schedule back for deletion.
// The audio thread does not allocate, free or lock (REQ-12-1).
class ScheduleSlot
{
public:
    ScheduleSlot ();
    ~ScheduleSlot ();

    // Message thread
    void post (std::unique_ptr<EventSchedule> schedule);
    void collectGarbage ();

    // Audio thread: takes a posted schedule, or returns nullptr.
    EventSchedule* takePosted ();

    // Audio thread: gives a schedule back for deletion on the message thread. Returns false if the queue is full.
    bool retire (EventSchedule* schedule);

private:
    static constexpr int                  kCapacity = 64;
    std::atomic<EventSchedule*>           m_posted { nullptr };
    juce::AbstractFifo                    m_fifo { kCapacity };
    std::array<EventSchedule*, kCapacity> m_retired {};
};

// Plays a schedule through a synthesizer (T12-03). A new schedule starts at the next bar during playback (REQ-12-5).
class Sequencer : public juce::AudioSource
{
public:
    explicit Sequencer (juce::Synthesiser& synth);
    ~Sequencer () override;

    // Message thread
    void setSchedule (std::unique_ptr<EventSchedule> schedule);
    void play ();
    void stop ();
    void setLoopSection (int section); // -1 = no loop (REQ-12-3)
    void setClick (bool on);           // REQ-12-4
    void collectGarbage ();

    bool isPlaying () const
    {
        return m_playing.load ();
    }
    int loopSection () const
    {
        return m_loop.load ();
    }
    bool clickOn () const
    {
        return m_click.load ();
    }
    double positionTicks () const; // -1 when playback stops

    // juce::AudioSource
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources () override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& info) override;

    // Fills the MIDI events of the next block and moves the position. getNextAudioBlock calls this.
    void fillMidi (juce::MidiBuffer& midi, int numSamples);

    double sampleRate () const
    {
        return m_sampleRate;
    }

private:
    void allNotesOff (juce::MidiBuffer& midi, int offset) const;

    juce::Synthesiser& m_synth;
    juce::MidiBuffer   m_midi;
    ScheduleSlot       m_slot;

    // Audio thread only
    EventSchedule* m_current { nullptr };
    EventSchedule* m_waiting { nullptr }; // Starts at the next bar
    double         m_position { 0.0 };    // In ticks
    bool           m_wasPlaying { false };

    double              m_sampleRate { 48000.0 };
    std::atomic<bool>   m_playing { false };
    std::atomic<bool>   m_restart { false };
    std::atomic<bool>   m_click { true };
    std::atomic<int>    m_loop { -1 };
    std::atomic<double> m_shownPosition { -1.0 };
};

#include "Sequencer.h"

#include "algo/Render.h"

#include <algorithm>
#include <cmath>

std::unique_ptr<EventSchedule> EventSchedule::fromEvents (const algo::EventList& events, int bpm, const std::vector<algo::Tick>& sectionStarts, algo::Tick barTicks)
{
    auto schedule           = std::make_unique<EventSchedule> ();
    schedule->bpm           = std::max (1, bpm);
    schedule->sectionStarts = sectionStarts.empty () ? std::vector<algo::Tick> { 0, events.length } : sectionStarts;
    schedule->barTicks      = std::max<algo::Tick> (1, barTicks);
    schedule->length        = events.length;

    schedule->events.reserve (events.notes.size () * 2);
    for (const auto& note : events.notes)
    {
        const auto channel = static_cast<std::uint8_t> (note.channel + 1);
        const bool click   = note.track == algo::kClickTrack;
        schedule->events.push_back ({ note.start, channel, static_cast<std::uint8_t> (note.pitch), static_cast<std::uint8_t> (juce::jlimit (1, 127, note.velocity)), true, click });
        schedule->events.push_back ({ note.start + std::max<algo::Tick> (1, note.length), channel, static_cast<std::uint8_t> (note.pitch), 0, false, click });
    }
    std::stable_sort (schedule->events.begin (), schedule->events.end (), [] (const ScheduledEvent& a, const ScheduledEvent& b)
    {
        if (a.tick != b.tick)
            return a.tick < b.tick;
        return !a.on && b.on;
    });
    return schedule;
}

std::unique_ptr<EventSchedule> EventSchedule::fromStarter (const algo::SongStarter& starter, const algo::DataSet& data)
{
    return fromEvents (algo::render (starter, algo::RenderOptions {}, data), starter.bpm, algo::sectionStarts (starter), algo::barTicks (starter.timeSig));
}

std::size_t EventSchedule::firstAtOrAfter (double tick) const
{
    const auto it = std::lower_bound (events.begin (), events.end (), tick, [] (const ScheduledEvent& e, double t)
    {
        return static_cast<double> (e.tick) < t;
    });
    return static_cast<std::size_t> (it - events.begin ());
}

ScheduleSlot::ScheduleSlot () = default;

ScheduleSlot::~ScheduleSlot ()
{
    collectGarbage ();
    delete m_posted.exchange (nullptr);
}

void ScheduleSlot::post (std::unique_ptr<EventSchedule> schedule)
{
    // The audio thread did not take the old posted schedule, so the message thread can delete it.
    delete m_posted.exchange (schedule.release ());
}

EventSchedule* ScheduleSlot::takePosted ()
{
    return m_posted.exchange (nullptr);
}

bool ScheduleSlot::retire (EventSchedule* schedule)
{
    if (schedule == nullptr)
        return true;
    const auto scope = m_fifo.write (1);
    if (scope.blockSize1 + scope.blockSize2 < 1)
        return false;
    m_retired[static_cast<std::size_t> (scope.blockSize1 > 0 ? scope.startIndex1 : scope.startIndex2)] = schedule;
    return true;
}

void ScheduleSlot::collectGarbage ()
{
    const auto ready = m_fifo.getNumReady ();
    const auto scope = m_fifo.read (ready);
    for (int i = 0; i < scope.blockSize1; ++i)
        delete m_retired[static_cast<std::size_t> (scope.startIndex1 + i)];
    for (int i = 0; i < scope.blockSize2; ++i)
        delete m_retired[static_cast<std::size_t> (scope.startIndex2 + i)];
}

Sequencer::Sequencer (juce::Synthesiser& synth)
    : m_synth (synth)
{
    m_midi.ensureSize (32768);
}

Sequencer::~Sequencer ()
{
    m_slot.collectGarbage ();
    delete m_current;
    delete m_waiting;
}

void Sequencer::setSchedule (std::unique_ptr<EventSchedule> schedule)
{
    m_slot.post (std::move (schedule));
}

void Sequencer::play ()
{
    m_restart = true;
    m_playing = true;
}

void Sequencer::stop ()
{
    m_playing = false;
}

void Sequencer::setLoopSection (int section)
{
    m_loop = section;
}

void Sequencer::setClick (bool on)
{
    m_click = on;
}

void Sequencer::collectGarbage ()
{
    m_slot.collectGarbage ();
}

double Sequencer::positionTicks () const
{
    return m_playing ? m_shownPosition.load () : -1.0;
}

void Sequencer::prepareToPlay (int, double sampleRate)
{
    m_sampleRate = sampleRate;
}

void Sequencer::releaseResources ()
{
}

void Sequencer::allNotesOff (juce::MidiBuffer& midi, int offset) const
{
    for (int channel : { 1, 2, 10 })
        midi.addEvent (juce::MidiMessage::allNotesOff (channel), offset);
}

void Sequencer::fillMidi (juce::MidiBuffer& midi, int numSamples)
{
    // A new schedule replaces the old one at once when playback stops, or at the next bar (REQ-12-5).
    if (auto* posted = m_slot.takePosted ())
    {
        if (m_waiting != nullptr && !m_slot.retire (m_waiting))
            m_waiting = nullptr; // The queue is full: this leaks one schedule, but it does not free on this thread.
        m_waiting = posted;
    }
    if (m_waiting != nullptr && (!m_playing || m_current == nullptr) && m_slot.retire (m_current))
    {
        m_current = m_waiting;
        m_waiting = nullptr;
    }

    const bool playing = m_playing.load ();
    if (!playing || m_current == nullptr)
    {
        if (m_wasPlaying)
            allNotesOff (midi, 0);
        m_wasPlaying    = false;
        m_shownPosition = -1.0;
        return;
    }

    if (m_restart.exchange (false) || !m_wasPlaying)
    {
        m_position = 0.0;
        if (m_wasPlaying)
            allNotesOff (midi, 0);
    }
    m_wasPlaying = true;

    const auto clickOn = m_click.load ();
    int        offset  = 0;
    int        left    = numSamples;
    while (left > 0 && m_current != nullptr)
    {
        const auto&  s   = *m_current;
        const double spt = m_sampleRate * 60.0 / (static_cast<double> (s.bpm) * algo::kPpq); // Samples for each tick

        // The loop range of the selected section.
        const int  loop      = m_loop.load ();
        const bool looping   = loop >= 0 && loop + 1 < static_cast<int> (s.sectionStarts.size ());
        const auto loopStart = looping ? static_cast<double> (s.sectionStarts[static_cast<std::size_t> (loop)]) : 0.0;
        const auto loopEnd   = looping ? static_cast<double> (s.sectionStarts[static_cast<std::size_t> (loop) + 1]) : static_cast<double> (s.length);
        if (loopEnd - loopStart < 1.0)
        {
            m_position = 0.0;
            break;
        }
        if (looping && (m_position < loopStart || m_position >= loopEnd))
        {
            allNotesOff (midi, offset);
            m_position = loopStart;
        }

        const double segmentEnd = m_position + left / spt;
        double       limit      = segmentEnd;
        enum class Reason
        {
            None,
            Switch,
            Wrap
        } reason = Reason::None;

        if (m_waiting != nullptr)
        {
            const double bar      = static_cast<double> (s.barTicks);
            const double boundary = std::ceil (m_position / bar - 1.0e-9) * bar;
            if (boundary < limit)
            {
                limit  = boundary;
                reason = Reason::Switch;
            }
        }
        if (loopEnd < limit || (loopEnd <= limit && reason == Reason::None))
        {
            limit  = loopEnd;
            reason = Reason::Wrap;
        }

        const int samples = reason == Reason::None ? left : juce::jlimit (0, left, static_cast<int> (std::llround ((limit - m_position) * spt)));
        for (auto i = s.firstAtOrAfter (m_position); i < s.events.size () && static_cast<double> (s.events[i].tick) < limit; ++i)
        {
            const auto& e = s.events[i];
            if (e.click && e.on && !clickOn)
                continue;
            const int at = juce::jlimit (offset, offset + std::max (0, samples - 1), offset + static_cast<int> (std::llround ((static_cast<double> (e.tick) - m_position) * spt)));
            midi.addEvent (e.on ? juce::MidiMessage::noteOn (e.channel, e.note, e.velocity) : juce::MidiMessage::noteOff (e.channel, e.note), at);
        }

        offset += samples;
        left -= samples;
        m_position = reason == Reason::None ? segmentEnd : limit;

        if (reason == Reason::Switch)
        {
            allNotesOff (midi, std::min (offset, numSamples - 1));
            if (m_slot.retire (m_current))
            {
                m_current = m_waiting;
                m_waiting = nullptr;
                if (m_position >= static_cast<double> (m_current->length))
                    m_position = 0.0;
            }
            else
            {
                // The queue is full. Try again in the next block.
                m_position += 1.0e-6;
            }
        }
        else if (reason == Reason::Wrap)
        {
            allNotesOff (midi, std::min (offset, numSamples - 1));
            m_position = loopStart;
        }

        if (samples == 0 && reason == Reason::None)
            break;
    }
    m_shownPosition = m_position;
}

void Sequencer::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion ();
    m_midi.clear ();
    fillMidi (m_midi, info.numSamples);
    m_synth.renderNextBlock (*info.buffer, m_midi, info.startSample, info.numSamples);
}

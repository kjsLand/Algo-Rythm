#include "algo/Render.h"

#include "algo/Fretboard.h"
#include "algo/Pitch.h"
#include "algo/Scale.h"
#include "algo/Seeds.h"
#include "algo/Strum.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <set>

namespace algo
{
    namespace
    {
        // The number of sharps of each major key, by pitch class. Pitch class 6 is F# (6) or Gb (-6).
        constexpr std::array<int, 12> kMajorSharps { 0, -5, 2, -3, 4, -1, 6, 1, -4, 3, -2, 5 };

        std::set<PitchClass> majorScale (PitchClass root)
        {
            std::set<PitchClass> notes;
            for (int step : { 0, 2, 4, 5, 7, 9, 11 })
                notes.insert (pitchClassOf (root + step));
            return notes;
        }

        int majorSharps (PitchClass root, bool flats)
        {
            if (root == 6)
                return flats ? -6 : 6;
            return kMajorSharps[static_cast<std::size_t> (root)];
        }

        // The tick range of the gap between two strings that obeys REQ-09-3.
        struct GapRange
        {
            Tick lo { 1 };
            Tick hi { 1 };
        };

        GapRange gapRange (int bpm)
        {
            const double ticksPerMs = bpm * kPpq / 60000.0;
            GapRange     range;
            range.lo = static_cast<Tick> (std::ceil (kMinStringGapMs * ticksPerMs - 1e-9));
            range.hi = std::max (range.lo, static_cast<Tick> (std::floor (kMaxStringGapMs * ticksPerMs + 1e-9)));
            return range;
        }

        // Renders the strokes of the bars of one section.
        class SectionRenderer
        {
        public:
            SectionRenderer (const SongStarter& starter, EventList& events, Rng& rng)
                : m_starter (starter),
                  m_events (events),
                  m_rng (rng),
                  m_fretboard (starter.tuning, starter.capo),
                  m_gaps (gapRange (starter.bpm))
            {
            }

            void renderBar (const Bar& bar, const StrumPattern& strum, Tick barStart)
            {
                const Tick length = barTicks (m_starter.timeSig);
                const Tick barEnd = barStart + length;
                const int  steps  = std::max (1, strum.steps);

                for (int slot = 0; slot < steps && slot < static_cast<int> (strum.slots.size ()); ++slot)
                {
                    const Tick   slotTick = barStart + length * slot / steps;
                    const Stroke stroke   = strum.slots[static_cast<std::size_t> (slot)];
                    const double accent   = slot < static_cast<int> (strum.accent.size ()) ? strum.accent[static_cast<std::size_t> (slot)] : 0.0;

                    if (stroke == Stroke::Rest)
                        continue;

                    if (stroke == Stroke::Chuck)
                    {
                        for (int s = 0; s < kNumStrings; ++s)
                            closeString (s, slotTick);
                        m_events.notes.push_back ({ kGuitarTrack, kDrumChannel, kChuckPitch, strumVelocity (accent), slotTick, std::max<Tick> (1, msToTicks (kMuteMs, m_starter.bpm)) });
                        continue;
                    }

                    if (bar.voicings.empty ())
                        continue;

                    const std::size_t chordIndex = std::min (bar.voicings.size () - 1, static_cast<std::size_t> (slot) * bar.voicings.size () / static_cast<std::size_t> (steps));
                    const auto        strings    = soundingStrings (bar.voicings[chordIndex], strum.strings, stroke == Stroke::Up);

                    Tick offset = 0;
                    for (std::size_t k = 0; k < strings.size (); ++k)
                    {
                        if (k > 0)
                            offset += nextGap ();

                        const int  s     = strings[k];
                        const Tick start = slotTick + offset;
                        closeString (s, start);

                        const int  fret  = bar.voicings[chordIndex].frets[static_cast<std::size_t> (s)];
                        const int  pitch = m_fretboard.midiNoteAt (s, fret);
                        const bool mute  = stroke == Stroke::Mute;

                        NoteEvent note { kGuitarTrack, kGuitarChannel, pitch, mute ? kMuteVelocity : strumVelocity (accent), start, 1 };
                        if (mute)
                        {
                            note.length = std::max<Tick> (1, msToTicks (kMuteMs, m_starter.bpm));
                            m_events.notes.push_back (note);
                        }
                        else
                        {
                            m_pending[static_cast<std::size_t> (s)] = m_events.notes.size ();
                            m_events.notes.push_back (note);
                        }
                    }
                }

                for (int s = 0; s < kNumStrings; ++s)
                    closeString (s, barEnd);
            }

        private:
            // The strings in the order of the stroke. Only the top `count` strings can sound.
            static std::vector<int> soundingStrings (const Voicing& voicing, int count, bool up)
            {
                std::vector<int> strings;
                const int        first = kNumStrings - std::clamp (count, 1, kNumStrings);
                for (int s = first; s < kNumStrings; ++s)
                    if (voicing.frets[static_cast<std::size_t> (s)] != kMutedString)
                        strings.push_back (s);

                if (up)
                    std::reverse (strings.begin (), strings.end ());
                return strings;
            }

            Tick nextGap ()
            {
                const double ms = kStringGapMs + (m_rng.uniformReal () * 2.0 - 1.0) * kStringJitterMs;
                return std::clamp (msToTicks (ms, m_starter.bpm), m_gaps.lo, m_gaps.hi);
            }

            void closeString (int s, Tick end)
            {
                auto& pending = m_pending[static_cast<std::size_t> (s)];
                if (!pending)
                    return;

                auto& note  = m_events.notes[*pending];
                note.length = std::max<Tick> (1, end - note.start);
                pending.reset ();
            }

            const SongStarter&                                  m_starter;
            EventList&                                          m_events;
            Rng&                                                m_rng;
            Fretboard                                           m_fretboard;
            GapRange                                            m_gaps;
            std::array<std::optional<std::size_t>, kNumStrings> m_pending {};
        };

        int beatsPerBar (const TimeSig& timeSig)
        {
            if (timeSig.denominator == 8 && timeSig.numerator % 3 == 0)
                return timeSig.numerator / 3;
            return std::max (1, timeSig.numerator);
        }
    }

    Tick barTicks (const TimeSig& timeSig)
    {
        return static_cast<Tick> (timeSig.numerator) * kPpq * 4 / std::max (1, timeSig.denominator);
    }

    std::vector<Tick> sectionStarts (const SongStarter& starter)
    {
        std::vector<Tick> starts { 0 };
        for (const auto& section : starter.sections)
            starts.push_back (starts.back () + barTicks (starter.timeSig) * static_cast<Tick> (section.bars.size ()));
        return starts;
    }

    int keySignatureSharps (const Key& key, const DataSet& data)
    {
        const auto                 notes = scalePitchClasses (key, data);
        const std::set<PitchClass> set (notes.begin (), notes.end ());

        for (PitchClass root = 0; root < 12; ++root)
            if (majorScale (root) == set)
                return majorSharps (root, usesFlats (key));

        const PitchClass parent = isMinorKey (key, data) ? pitchClassOf (key.root + 3) : key.root;
        return majorSharps (parent, usesFlats (Key { parent, "Major" }));
    }

    bool isMinorKey (const Key& key, const DataSet& data)
    {
        return isMinorScale (key.scale, data);
    }

    int strumVelocity (double accent)
    {
        return std::clamp (static_cast<int> (std::lround (70.0 + 40.0 * accent)), 1, 127);
    }

    EventList render (const SongStarter& starter, const RenderOptions& options, const DataSet& data)
    {
        EventList events;
        events.metas.push_back (tempoEvent (starter.bpm));
        events.metas.push_back (timeSignatureEvent (starter.timeSig));
        events.metas.push_back (keySignatureEvent (keySignatureSharps (starter.key, data), isMinorKey (starter.key, data)));
        events.metas.push_back (trackNameEvent (kGuitarTrack, "Guitar"));
        events.metas.push_back (trackNameEvent (kHookTrack, "Hook"));
        events.metas.push_back (trackNameEvent (kClickTrack, "Click"));

        const auto starts = sectionStarts (starter);
        const Tick bar    = barTicks (starter.timeSig);
        const int  beats  = beatsPerBar (starter.timeSig);

        // In section mode, the chosen section starts at tick 0.
        const bool oneSection = options.section >= 0;
        const Tick shift      = oneSection ? starts.at (static_cast<std::size_t> (options.section)) : 0;

        for (std::size_t i = 0; i < starter.sections.size (); ++i)
        {
            if (oneSection && static_cast<int> (i) != options.section)
                continue;

            const auto& section = starter.sections[i];
            const Tick  base    = starts[i] - shift;
            events.metas.push_back (markerEvent (base, section.name));

            Rng             rng = rngFor (starter.seed, Stage::Render, static_cast<int> (i), 0);
            SectionRenderer renderer (starter, events, rng);
            for (std::size_t b = 0; b < section.bars.size (); ++b)
            {
                const Tick barStart = base + bar * static_cast<Tick> (b);
                renderer.renderBar (section.bars[b], section.strum, barStart);

                if (options.click)
                    for (int beat = 0; beat < beats; ++beat)
                        events.notes.push_back ({ kClickTrack, kDrumChannel, beat == 0 ? kClickHighPitch : kClickLowPitch, beat == 0 ? 100 : 80, barStart + bar * beat / beats, kClickLength });
            }
        }

        events.length = oneSection ? starts.at (static_cast<std::size_t> (options.section) + 1) - shift : starts.back ();

        if (options.hook)
        {
            for (const auto& note : starter.hook)
            {
                const Tick start = note.start - shift;
                if (start < 0 || start >= events.length)
                    continue;
                events.notes.push_back ({ kHookTrack, kHookChannel, note.pitch, note.velocity, start, std::min (note.length, events.length - start) });
            }
        }

        std::stable_sort (events.notes.begin (), events.notes.end (), [] (const NoteEvent& a, const NoteEvent& b)
        {
            return a.start < b.start;
        });
        return events;
    }
}

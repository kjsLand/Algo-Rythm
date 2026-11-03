#include "algo/Pitch.h"
#include "algo/Voicing.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace algo
{
    namespace
    {
        constexpr int kWindowFrets = 3; // A window goes from the base fret to the base fret + 3
        constexpr int kMaxBaseFret = 12;

        int maskOf (PitchClass pc)
        {
            return 1 << pc;
        }

        int bitCount (int mask)
        {
            int count = 0;
            for (; mask != 0; mask &= mask - 1)
                ++count;
            return count;
        }

        // A depth-first search from the low E string to the high E string. The pitch classes are bit masks.
        class Search
        {
        public:
            Search (const ConcreteChord& chord, const Fretboard& fretboard, Difficulty difficulty)
                : m_lowestPc (chord.bass ? *chord.bass : chord.root),
                  m_canMuteBetween (difficulty == Difficulty::Advanced)
            {
                int allowed = maskOf (chord.root) | maskOf (m_lowestPc);
                m_required  = allowed;

                for (int interval : chord.definition.intervals)
                {
                    allowed |= maskOf (pitchClassOf (chord.root + interval));

                    const auto& optional = chord.definition.optional;
                    if (std::find (optional.begin (), optional.end (), interval) == optional.end ())
                        m_required |= maskOf (pitchClassOf (chord.root + interval));
                }

                for (std::size_t s = 0; s < m_notes.size (); ++s)
                    for (int f = 0; f <= kMaxCandidateFret; ++f)
                    {
                        const MidiNote note = fretboard.midiNoteAt (static_cast<int> (s), f);

                        m_notes[s][static_cast<std::size_t> (f)]   = note;
                        m_isChord[s][static_cast<std::size_t> (f)] = (allowed & maskOf (pitchClassOf (note))) != 0;
                    }
            }

            std::vector<Voicing> run ()
            {
                for (m_baseFret = 0; m_baseFret <= kMaxBaseFret; ++m_baseFret)
                    visit (0, 0, 0, false);

                // A voicing in frets 4 to 6 is in the windows of base frets 3 and 4.
                std::sort (m_results.begin (), m_results.end (), [] (const Voicing& a, const Voicing& b)
                {
                    return a.frets < b.frets;
                });
                m_results.erase (std::unique (m_results.begin (), m_results.end (), [] (const Voicing& a, const Voicing& b)
                {
                    return a.frets == b.frets;
                }),
                                 m_results.end ());
                return std::move (m_results);
            }

        private:
            void visit (int string, int soundCount, int found, bool stopped)
            {
                const int remaining = kNumStrings - string;

                // Stop a branch that cannot have enough strings or all of the required notes.
                if (soundCount + (stopped ? 0 : remaining) < kMinStrummedStrings)
                    return;
                if (bitCount (m_required & ~found) > (stopped ? 0 : remaining))
                    return;

                if (string == kNumStrings)
                {
                    finish ();
                    return;
                }

                const auto s = static_cast<std::size_t> (string);

                m_current.frets[s] = kMutedString;
                visit (string + 1, soundCount, found, stopped || (soundCount > 0 && !m_canMuteBetween));

                if (stopped)
                    return;

                if (m_isChord[s][0])
                    play (string, 0, soundCount, found);

                for (int fret = std::max (m_baseFret, 1); fret <= m_baseFret + kWindowFrets; ++fret)
                    if (m_isChord[s][static_cast<std::size_t> (fret)])
                        play (string, fret, soundCount, found);

                m_current.frets[s] = kMutedString;
            }

            void play (int string, int fret, int soundCount, int found)
            {
                const auto s = static_cast<std::size_t> (string);

                m_current.frets[s] = fret;
                visit (string + 1, soundCount + 1, found | maskOf (pitchClassOf (m_notes[s][static_cast<std::size_t> (fret)])), false);
            }

            void finish ()
            {
                MidiNote lowest = 128;
                for (std::size_t s = 0; s < m_current.frets.size (); ++s)
                    if (m_current.frets[s] != kMutedString)
                        lowest = std::min (lowest, m_notes[s][static_cast<std::size_t> (m_current.frets[s])]);

                if (pitchClassOf (lowest) == m_lowestPc && fingerCount (m_current) <= kMaxFingers)
                    m_results.push_back (m_current);
            }

            PitchClass m_lowestPc;
            bool       m_canMuteBetween;
            int        m_required { 0 };
            int        m_baseFret { 0 };
            Voicing    m_current;

            std::array<std::array<MidiNote, kMaxCandidateFret + 1>, kNumStrings> m_notes {};
            std::array<std::array<bool, kMaxCandidateFret + 1>, kNumStrings>     m_isChord {};
            std::vector<Voicing>                                                 m_results;
        };
    }

    std::vector<Voicing> searchCandidates (const ConcreteChord& chord, const Fretboard& fretboard, Difficulty difficulty)
    {
        return Search (chord, fretboard, difficulty).run ();
    }
}

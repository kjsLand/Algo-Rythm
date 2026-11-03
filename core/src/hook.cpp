#include "algo/Hook.h"

#include "algo/Diatonic.h"
#include "algo/Euclid.h"
#include "algo/Pitch.h"
#include "algo/Render.h"
#include "algo/Scale.h"
#include "algo/Voicing.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <stdexcept>

namespace algo
{
    namespace
    {
        bool isSixEight (const TimeSig& timeSig)
        {
            return timeSig.numerator == 6 && timeSig.denominator == 8;
        }

        const ChordSymbol* chordAt (const std::vector<Bar>& bars, Tick tick, const TimeSig& timeSig)
        {
            const Tick bar = barTicks (timeSig);
            if (bars.empty () || tick < 0)
                return nullptr;

            const auto  index = std::min (bars.size () - 1, static_cast<std::size_t> (tick / bar));
            const auto& b     = bars[index];
            if (b.chords.empty ())
                return nullptr;

            const Tick inBar = tick - static_cast<Tick> (index) * bar;
            const auto chord = std::min (b.chords.size () - 1, static_cast<std::size_t> (inBar) * b.chords.size () / static_cast<std::size_t> (bar));
            return &b.chords[chord];
        }

        bool contains (const std::vector<PitchClass>& pcs, MidiNote note)
        {
            return std::find (pcs.begin (), pcs.end (), pitchClassOf (note)) != pcs.end ();
        }

        // The nearest note in [lo, hi] with a pitch class in the list. Two notes at the same distance give the lower note.
        MidiNote nearestIn (MidiNote note, MidiNote lo, MidiNote hi, const std::vector<PitchClass>& pcs)
        {
            note = std::clamp (note, lo, hi);
            for (int d = 0; d <= hi - lo; ++d)
            {
                if (note - d >= lo && contains (pcs, note - d))
                    return note - d;
                if (note + d <= hi && contains (pcs, note + d))
                    return note + d;
            }
            return note;
        }
    }

    std::vector<Tick> hookRhythm (int bars, const TimeSig& timeSig, Rng& rng)
    {
        const Tick bar   = barTicks (timeSig);
        const int  steps = static_cast<int> (bar / kHookStepTicks);

        std::vector<Tick> starts;
        for (int b = 0; b < bars; ++b)
        {
            const int  hits    = std::min (steps, rng.uniformInt (kMinHookHits, kMaxHookHits));
            const auto pattern = rotate (bjorklund (hits, steps), rng.uniformInt (0, 3));
            for (int step = 0; step < steps; ++step)
                if (pattern[static_cast<std::size_t> (step)] == 1)
                    starts.push_back (static_cast<Tick> (b) * bar + static_cast<Tick> (step) * kHookStepTicks);
        }
        return starts;
    }

    bool isStrongBeat (Tick tickInBar, const TimeSig& timeSig)
    {
        if (isSixEight (timeSig))
            return tickInBar == 0 || tickInBar == barTicks (timeSig) / 2;
        return tickInBar == 0 || tickInBar == 2 * kPpq;
    }

    int hookSectionIndex (const std::vector<std::string>& roles)
    {
        const auto chorus = std::find (roles.begin (), roles.end (), "chorus");
        return chorus == roles.end () ? 0 : static_cast<int> (chorus - roles.begin ());
    }

    MidiNote hookRangeLow (const Key& key, const DataSet& data)
    {
        const MidiNote center = 60 + degreePitchClass (key, 5, data);
        return center - kHookRange / 2;
    }

    std::vector<PitchClass> hookChordTones (const std::vector<Bar>& bars, Tick tickInSection, const TimeSig& timeSig, const Key& key, const DataSet& data)
    {
        const auto* symbol = chordAt (bars, tickInSection, timeSig);
        if (symbol == nullptr)
            return {};

        const auto              chord = concreteChord (*symbol, key, data);
        std::vector<PitchClass> tones;
        for (auto pc : ChordRegistry::pitchClasses (chord.root, chord.definition))
            if (isInScale (pc, key, data))
                tones.push_back (pc);
        return tones;
    }

    std::vector<PitchClass> hookEndTones (const std::vector<Bar>& bars, Tick tickInSection, const TimeSig& timeSig, const Key& key, const DataSet& data)
    {
        const auto* symbol = chordAt (bars, tickInSection, timeSig);
        if (symbol == nullptr)
            return {};

        const auto chord = concreteChord (*symbol, key, data);
        const auto pcs   = ChordRegistry::pitchClasses (chord.root, chord.definition);

        std::vector<PitchClass> tones;
        if (isInScale (chord.root, key, data))
            tones.push_back (chord.root);

        // The 3rd is the second note of the chord, if it is a third above the root.
        if (pcs.size () > 1)
        {
            const int third = pitchClassOf (pcs[1] - chord.root);
            if ((third == 3 || third == 4) && isInScale (pcs[1], key, data))
                tones.push_back (pcs[1]);
        }

        return tones;
    }

    std::vector<MelodyNote> makeHook (const SongPlan& plan, const ProgressionSet& progressions, const RngFactory& rngFactory, const DataSet& data)
    {
        if (progressions.sections.empty ())
            return {};

        std::vector<std::string> roles;
        for (const auto& section : progressions.sections)
            roles.push_back (section.role);

        const auto  sectionIndex = static_cast<std::size_t> (hookSectionIndex (roles));
        const auto& section      = progressions.sections[sectionIndex];
        const Tick  bar          = barTicks (plan.timeSig);

        Tick sectionStart = 0;
        for (std::size_t i = 0; i < sectionIndex; ++i)
            sectionStart += bar * static_cast<Tick> (progressions.sections[i].bars.size ());

        Rng rng = rngFactory (Stage::Hook, 0);

        const int  bars   = std::min (rng.uniformInt (kMinHookBars, kMaxHookBars), static_cast<int> (section.bars.size ()));
        const auto starts = hookRhythm (bars, plan.timeSig, rng);
        if (starts.empty ())
            return {};

        const auto contour = randomContour (rng);
        const Tick total   = bar * bars;

        std::vector<double> values;
        for (auto start : starts)
            values.push_back (evaluate (contour, static_cast<double> (start) / static_cast<double> (total)));

        const auto [lowIt, highIt] = std::minmax_element (values.begin (), values.end ());
        const double low           = *lowIt;
        const double span          = *highIt - low;

        const MidiNote lo    = hookRangeLow (plan.key, data);
        const MidiNote hi    = lo + kHookRange;
        const auto     scale = scalePitchClasses (plan.key, data);

        std::vector<MelodyNote> notes;
        for (std::size_t i = 0; i < starts.size (); ++i)
        {
            const double   normal = span > 0.0 ? (values[i] - low) / span : 0.5;
            const MidiNote raw    = lo + static_cast<int> (std::lround (normal * kHookRange));
            const Tick     inBar  = starts[i] % bar;
            const bool     strong = isStrongBeat (inBar, plan.timeSig);

            auto tones = strong ? hookChordTones (section.bars, starts[i], plan.timeSig, plan.key, data) : scale;
            if (tones.empty ())
                tones = scale;

            MelodyNote note;
            note.pitch    = nearestIn (raw, lo, hi, tones);
            note.start    = starts[i];
            note.velocity = strong ? kStrongHookVelocity : kWeakHookVelocity;
            notes.push_back (note);
        }

        // The last note goes to the nearest root or 3rd of the last chord.
        auto& last = notes.back ();
        // A borrowed chord can have no root or 3rd in the scale. Then the note stays a chord tone in the scale (REQ-13-1).
        auto endTones = hookEndTones (section.bars, last.start, plan.timeSig, plan.key, data);
        if (endTones.empty ())
            endTones = hookChordTones (section.bars, last.start, plan.timeSig, plan.key, data);
        if (endTones.empty ())
            endTones = scale;
        last.pitch = nearestIn (last.pitch, lo, hi, endTones);

        for (std::size_t i = 0; i < notes.size (); ++i)
        {
            const Tick next = i + 1 < notes.size () ? notes[i + 1].start : total;
            notes[i].length = i + 1 < notes.size () ? std::min (next - notes[i].start, kMaxHookNote) : kLastHookNote;
            notes[i].start += sectionStart;
        }
        return notes;
    }
}

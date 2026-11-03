#include "algo/Strum.h"

#include "algo/Euclid.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace algo
{
    namespace
    {
        std::size_t at (int index)
        {
            return static_cast<std::size_t> (index);
        }

        bool isSixEight (const TimeSig& timeSig)
        {
            return timeSig.numerator == 6 && timeSig.denominator == 8;
        }

        // The slots that a rest mask must not cover.
        std::vector<int> protectedSlots (int steps, Difficulty difficulty)
        {
            std::vector<int> slots { 0 };
            if (difficulty == Difficulty::Beginner)
                slots.push_back (steps / 2);
            return slots;
        }

        bool maskIsValid (const std::vector<int>& mask, const std::vector<int>& slots)
        {
            return std::none_of (slots.begin (), slots.end (), [&mask] (int slot)
            {
                return mask[at (slot)] == 1;
            });
        }

        // Rotates the mask until no protected slot is a rest. If no rotation is valid, removes one rest and tries again.
        std::vector<int> placeRestMask (std::vector<int> mask, const std::vector<int>& slots)
        {
            const int steps = static_cast<int> (mask.size ());
            while (true)
            {
                for (int r = 0; r < steps; ++r)
                {
                    if (maskIsValid (mask, slots))
                        return mask;
                    mask = rotate (mask, 1);
                }

                const auto last = std::find (mask.rbegin (), mask.rend (), 1);
                if (last == mask.rend ())
                    return mask;
                *last = 0;
            }
        }

        Stroke strokeFor (int slot)
        {
            return slot % 2 == 0 ? Stroke::Down : Stroke::Up;
        }

        bool isBeat (int slot, int steps, const TimeSig& timeSig)
        {
            const auto beats = beatSlots (steps, timeSig);
            return std::find (beats.begin (), beats.end (), slot) != beats.end ();
        }

        // A lower value is a better slot for a new hit: a beat, then an even slot, then an odd slot.
        int addPriority (int slot, int steps, const TimeSig& timeSig)
        {
            if (isBeat (slot, steps, timeSig))
                return 0;
            return slot % 2 == 0 ? 1 : 2;
        }

        void fitHitCount (StrumPattern& pattern, int target, const std::vector<int>& keep, const TimeSig& timeSig)
        {
            const int steps = pattern.steps;

            std::vector<int> order (at (steps));
            for (int i = 0; i < steps; ++i)
                order[at (i)] = i;
            std::stable_sort (order.begin (), order.end (), [&] (int a, int b)
            {
                return addPriority (a, steps, timeSig) < addPriority (b, steps, timeSig);
            });

            for (int slot : order)
            {
                if (hitCount (pattern) >= target)
                    break;
                if (pattern.slots[at (slot)] == Stroke::Rest)
                    pattern.slots[at (slot)] = strokeFor (slot);
            }

            // Remove the least important hits first, from the end of the bar.
            for (auto it = order.rbegin (); it != order.rend () && hitCount (pattern) > target; ++it)
            {
                if (std::find (keep.begin (), keep.end (), *it) != keep.end ())
                    continue;
                pattern.slots[at (*it)] = Stroke::Rest;
            }
        }
    }

    int gridSteps (Difficulty difficulty, int bpm, const TimeSig& timeSig)
    {
        if (isSixEight (timeSig))
            return 12;
        if (difficulty == Difficulty::Beginner || bpm > kFastStrumBpm)
            return 8;
        return 16;
    }

    std::vector<int> beatSlots (int steps, const TimeSig& timeSig)
    {
        const int beats   = isSixEight (timeSig) ? 2 : std::max (1, timeSig.numerator);
        const int perBeat = std::max (1, steps / beats);

        std::vector<int> slots;
        for (int beat = 0; beat < beats; ++beat)
            slots.push_back (beat * perBeat);
        return slots;
    }

    int hitCount (const StrumPattern& pattern)
    {
        return static_cast<int> (std::count_if (pattern.slots.begin (), pattern.slots.end (), [] (Stroke stroke)
        {
            return !isSilent (stroke);
        }));
    }

    bool isSilent (Stroke stroke)
    {
        return stroke == Stroke::Rest || stroke == Stroke::Chuck;
    }

    std::pair<int, int> baseHitRange (int steps)
    {
        const int lo = std::max (1, steps * 3 / 8);
        const int hi = std::max (lo, std::min (steps - 2, steps * 5 / 8));
        return { lo, hi };
    }

    int chooseBaseHits (int steps, Rng& rng)
    {
        const auto [lo, hi] = baseHitRange (steps);
        return rng.uniformInt (lo, hi);
    }

    StrumPattern euclidStrum (int steps, int targetHits, Difficulty difficulty, Rng& rng, const TimeSig& timeSig)
    {
        if (steps < 2)
            throw std::invalid_argument ("A strum grid needs 2 or more steps: " + std::to_string (steps));

        const int target    = std::clamp (targetHits, 1, steps);
        const int restHits  = rng.uniformInt (0, steps / 4);
        const int layerHits = std::min (steps, target + restHits);

        // Layer 1: the strum layer. Slot 0 is always a hit.
        auto strum = rotate (bjorklund (layerHits, steps), rng.uniformInt (0, steps - 1));
        strum[0]   = 1;

        // Layer 2: the rest mask.
        const auto keep = protectedSlots (steps, difficulty);
        const auto mask = placeRestMask (bjorklund (restHits, steps), keep);

        StrumPattern pattern;
        pattern.steps = steps;
        pattern.slots.assign (at (steps), Stroke::Rest);
        pattern.accent.assign (at (steps), 0.0);
        for (int i = 0; i < steps; ++i)
            if (strum[at (i)] == 1 && mask[at (i)] == 0)
                pattern.slots[at (i)] = strokeFor (i);

        for (int slot : keep)
            pattern.slots[at (slot)] = strokeFor (slot);

        fitHitCount (pattern, std::max (target, static_cast<int> (keep.size ())), keep, timeSig);
        return pattern;
    }

    void setAccents (StrumPattern& pattern, const TimeSig& timeSig)
    {
        const auto beats = beatSlots (pattern.steps, timeSig);

        pattern.accent.assign (at (pattern.steps), 0.0);
        for (int i = 0; i < pattern.steps; ++i)
        {
            const auto stroke = pattern.slots[at (i)];
            if (stroke == Stroke::Rest)
                continue;

            const auto beat        = std::find (beats.begin (), beats.end (), i);
            const bool backbeat    = beat != beats.end () && (beat - beats.begin ()) % 2 == 1;
            pattern.accent[at (i)] = backbeat ? kBackbeatAccent : kHitAccent;
        }
    }

    void addPercussion (StrumPattern& pattern, Difficulty difficulty, Rng& rng, const StrumRules& rules, const TimeSig& timeSig)
    {
        if (difficulty != Difficulty::Advanced)
            return;

        const auto beats = beatSlots (pattern.steps, timeSig);
        for (int i = 1; i < pattern.steps; ++i)
        {
            auto&      stroke   = pattern.slots[at (i)];
            const auto beat     = std::find (beats.begin (), beats.end (), i);
            const bool backbeat = beat != beats.end () && (beat - beats.begin ()) % 2 == 1;

            if (stroke == Stroke::Rest && backbeat && rng.chance (rules.chuckChance))
                stroke = Stroke::Chuck;
            else if (stroke == Stroke::Up && rng.chance (rules.muteChance))
                stroke = Stroke::Mute;
        }
    }

    StrumPattern makeSectionStrum (const SongPlan& plan, const std::string& role, int baseHits, Rng& rng, const StrumLibrary& library, const StrumRules& rules)
    {
        const int steps = gridSteps (plan.difficulty, plan.bpm, plan.timeSig);

        int target = baseHits;
        if (role == "chorus")
            target = std::min (steps, baseHits + rng.uniformInt (1, 2));

        StrumPattern pattern;
        bool         fromLibrary = false;
        if (rng.chance (rules.libraryChance) && !isSixEight (plan.timeSig))
        {
            if (const auto def = library.pickLibraryPattern (plan.difficulty, steps, target, rng))
            {
                pattern     = def->pattern;
                fromLibrary = true;
            }
        }
        if (!fromLibrary)
            pattern = euclidStrum (steps, target, plan.difficulty, rng, plan.timeSig);

        pattern.strings = plan.difficulty == Difficulty::Beginner || rng.chance (0.5) ? kNumStrings : kPartialStrumStrings;

        addPercussion (pattern, plan.difficulty, rng, rules, plan.timeSig);
        setAccents (pattern, plan.timeSig);
        return pattern;
    }

    int songBaseHits (const SongPlan& plan, const RngFactory& rngFactory)
    {
        Rng rng = rngFactory (Stage::Strum, kSongStrumSection);
        return chooseBaseHits (gridSteps (plan.difficulty, plan.bpm, plan.timeSig), rng);
    }

    StrumPattern makeSectionStrum (const SongPlan& plan, int sectionIndex, const RngFactory& rngFactory, const StrumLibrary& library, const StrumRules& rules)
    {
        const int baseHits = songBaseHits (plan, rngFactory);
        Rng       rng      = rngFactory (Stage::Strum, sectionIndex);
        return makeSectionStrum (plan, plan.sections.at (at (sectionIndex)).role, baseHits, rng, library, rules);
    }

    std::vector<StrumPattern> makeStrums (const SongPlan& plan, const RngFactory& rngFactory, const StrumLibrary& library, const StrumRules& rules)
    {
        const int baseHits = songBaseHits (plan, rngFactory);

        std::vector<StrumPattern> patterns;
        for (std::size_t i = 0; i < plan.sections.size (); ++i)
        {
            Rng rng = rngFactory (Stage::Strum, static_cast<int> (i));
            patterns.push_back (makeSectionStrum (plan, plan.sections[i].role, baseHits, rng, library, rules));
        }
        return patterns;
    }

    std::vector<StrumPattern> makeStrums (const SongPlan& plan, const RngFactory& rngFactory, const DataSet& data)
    {
        return makeStrums (plan, rngFactory, StrumLibrary::fromData (data));
    }

    std::string strumText (const StrumPattern& pattern)
    {
        std::string text;
        for (std::size_t i = 0; i < pattern.slots.size (); ++i)
        {
            if (i > 0)
                text += ' ';
            text += strokeChar (pattern.slots[i]);
        }
        return text;
    }

    std::string strumRuler (const StrumPattern& pattern, const TimeSig& timeSig)
    {
        std::string text;
        for (int i = 0; i < pattern.steps; ++i)
        {
            if (i > 0)
                text += ' ';

            if (isSixEight (timeSig))
            {
                text += i % 2 == 0 ? static_cast<char> ('1' + i / 2) : '+';
                continue;
            }

            const int beats   = std::max (1, timeSig.numerator);
            const int perBeat = std::max (1, pattern.steps / beats);
            const int inBeat  = i % perBeat;
            if (inBeat == 0)
                text += static_cast<char> ('1' + i / perBeat);
            else if (perBeat == 4)
                text += inBeat == 1 ? 'e' : (inBeat == 2 ? '+' : 'a');
            else
                text += '+';
        }
        return text;
    }

    // StrumLibrary

    StrumLibrary::StrumLibrary (std::vector<StrumDefinition> definitions)
        : m_definitions (std::move (definitions))
    {
    }

    StrumLibrary StrumLibrary::fromData (const DataSet& data)
    {
        StrumLibrary library (data.strums);
        library.setWeights (data.strumWeights);
        return library;
    }

    void StrumLibrary::setWeights (const std::map<std::string, double>& weights)
    {
        for (const auto& [name, weight] : weights)
            if (!(weight > 0.0))
                throw std::invalid_argument ("The strum weight of '" + name + "' must be positive");
        m_weights = weights;
    }

    double StrumLibrary::weight (const std::string& name) const
    {
        const auto it = m_weights.find (name);
        return it == m_weights.end () ? 1.0 : it->second;
    }

    std::vector<StrumDefinition> StrumLibrary::filter (Difficulty difficulty, int steps, int targetHits) const
    {
        std::vector<StrumDefinition> result;
        for (const auto& def : m_definitions)
            if (def.difficulty <= difficulty && def.pattern.steps == steps && hitCount (def.pattern) == targetHits)
                result.push_back (def);
        return result;
    }

    std::optional<StrumDefinition> StrumLibrary::pickLibraryPattern (Difficulty difficulty, int steps, int targetHits, Rng& rng) const
    {
        const auto candidates = filter (difficulty, steps, targetHits);
        if (candidates.empty ())
            return std::nullopt;

        std::vector<double> weights;
        for (const auto& def : candidates)
            weights.push_back (weight (def.name));
        return candidates[rng.weightedIndex (weights)];
    }
}

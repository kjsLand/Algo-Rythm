#include "algo/Engine.h"

#include "algo/DataError.h"
#include "algo/Fretboard.h"
#include "algo/Hook.h"
#include "algo/Render.h"
#include "algo/Strum.h"
#include "algo/VoicingCache.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace algo
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        double msSince (Clock::time_point start)
        {
            return std::chrono::duration<double, std::milli> (Clock::now () - start).count ();
        }

        int countAt (const std::vector<int>& counts, int index)
        {
            return index >= 0 && index < static_cast<int> (counts.size ()) ? counts[static_cast<std::size_t> (index)] : 0;
        }

        std::vector<int>* countsFor (RerollCounts& counts, Stage stage)
        {
            switch (stage)
            {
                case Stage::Chords: return &counts.chords;
                case Stage::Voicing: return &counts.voicings;
                case Stage::Strum: return &counts.strums;
                default: return nullptr;
            }
        }

        std::string joinErrors (const std::vector<std::string>& errors)
        {
            std::string text = "The settings are not valid:";
            for (const auto& error : errors)
                text += "\n  " + error;
            return text;
        }

        std::string readFile (const std::filesystem::path& path)
        {
            std::ifstream stream (path, std::ios::binary);
            if (!stream)
                throw DataError (path.filename ().string (), "", "the file cannot be read");
            std::ostringstream text;
            text << stream.rdbuf ();
            return text.str ();
        }

        // The values that a lock copies from the previous song starter.
        void applyPlanLocks (SongPlan& plan, const GenerationSettings& settings)
        {
            if (!settings.previous)
                return;

            const auto& previous = *settings.previous;
            const auto& locks    = settings.locks;
            if (locks.key)
            {
                plan.key.root = previous.key.root;
                plan.capo     = previous.capo;
            }
            if (locks.scale)
                plan.key.scale = previous.key.scale;
            if (locks.tempo)
                plan.bpm = previous.bpm;
            if (locks.form)
            {
                plan.form = previous.form;
                plan.sections.clear ();
                for (const auto& section : previous.sections)
                    plan.sections.push_back ({ section.name, section.role.empty () ? "verse" : section.role, static_cast<int> (section.bars.size ()) });
            }
        }

        SectionLocks sectionLock (const GenerationSettings& settings, std::size_t index)
        {
            if (!settings.previous || index >= settings.locks.sections.size () || index >= settings.previous->sections.size ())
                return {};
            return settings.locks.sections[index];
        }

        std::vector<ChordSymbol> flatChords (const std::vector<Bar>& bars)
        {
            std::vector<ChordSymbol> chords;
            for (const auto& bar : bars)
                chords.insert (chords.end (), bar.chords.begin (), bar.chords.end ());
            return chords;
        }

        // Copies the locked chords into the section. The loop repeats to fill a new bar count.
        void lockChords (SectionChords& section, const Section& previous, int perBar)
        {
            const auto chords = flatChords (previous.bars);
            if (chords.empty ())
                return;

            section.loop.assign (chords.begin (), chords.begin () + static_cast<std::ptrdiff_t> (std::min<std::size_t> (chords.size (), kLoopLength)));

            std::size_t next = 0;
            for (auto& bar : section.bars)
            {
                bar.chords.clear ();
                for (int i = 0; i < perBar; ++i)
                    bar.chords.push_back (chords[next++ % chords.size ()]);
            }
        }

        bool sameChords (const std::vector<Bar>& a, const std::vector<Bar>& b)
        {
            if (a.size () != b.size ())
                return false;
            for (std::size_t i = 0; i < a.size (); ++i)
                if (!(a[i].chords == b[i].chords))
                    return false;
            return true;
        }

        Fretboard fretboardFor (const SongStarter& starter)
        {
            return Fretboard (starter.tuning, starter.capo);
        }
    }

    int rerollCount (const RerollCounts& counts, Stage stage, int sectionIndex)
    {
        switch (stage)
        {
            case Stage::Chords: return countAt (counts.chords, sectionIndex);
            case Stage::Voicing: return countAt (counts.voicings, sectionIndex);
            case Stage::Strum: return countAt (counts.strums, sectionIndex);
            case Stage::Hook: return counts.hook;
            case Stage::Plan:
            case Stage::Render: return 0;
        }
        return 0;
    }

    RngFactory makeRngFactory (std::uint64_t masterSeed, const RerollCounts& counts)
    {
        return [masterSeed, counts] (Stage stage, int sectionIndex)
        {
            return rngFor (masterSeed, stage, sectionIndex, rerollCount (counts, stage, sectionIndex));
        };
    }

    SongStarter assemble (const SongPlan& plan, const ProgressionSet& progressions, const std::vector<StrumPattern>& strums, const std::vector<MelodyNote>& hook, const Tuning& tuning, std::uint64_t seed)
    {
        if (progressions.sections.size () != strums.size ())
            throw GenerationError ("The chord stage and the strum stage have different section counts");

        SongStarter starter;
        starter.seed       = seed;
        starter.key        = plan.key;
        starter.bpm        = plan.bpm;
        starter.timeSig    = plan.timeSig;
        starter.capo       = plan.capo;
        starter.tuning     = tuning;
        starter.difficulty = plan.difficulty;
        starter.form       = plan.form;
        starter.hook       = hook;

        for (std::size_t i = 0; i < progressions.sections.size (); ++i)
        {
            Section section;
            section.name  = progressions.sections[i].name;
            section.role  = progressions.sections[i].role;
            section.bars  = progressions.sections[i].bars;
            section.strum = strums[i];
            starter.sections.push_back (std::move (section));
        }
        return starter;
    }

    SongPlan planOf (const SongStarter& starter)
    {
        SongPlan plan;
        plan.key        = starter.key;
        plan.bpm        = starter.bpm;
        plan.timeSig    = starter.timeSig;
        plan.capo       = starter.capo;
        plan.difficulty = starter.difficulty;
        plan.form       = starter.form;
        for (const auto& section : starter.sections)
            plan.sections.push_back ({ section.name, section.role.empty () ? "verse" : section.role, static_cast<int> (section.bars.size ()) });
        return plan;
    }

    ProgressionSet progressionsOf (const SongStarter& starter)
    {
        ProgressionSet set;
        for (const auto& section : starter.sections)
        {
            SectionChords chords;
            chords.name = section.name;
            chords.role = section.role;
            chords.bars = section.bars;

            const auto flat = flatChords (section.bars);
            chords.loop.assign (flat.begin (), flat.begin () + static_cast<std::ptrdiff_t> (std::min<std::size_t> (flat.size (), kLoopLength)));
            set.sections.push_back (std::move (chords));
        }
        return set;
    }

    DataSet dataForTuning (const DataSet& data, const std::string& tuning)
    {
        DataSet copy = data;
        if (tuning != "Standard")
            copy.shapes.erase (std::remove_if (copy.shapes.begin (), copy.shapes.end (), [] (const ChordShape& shape)
            {
                return shape.open;
            }),
                               copy.shapes.end ());
        return copy;
    }

    SongStarter generate (const GenerationSettings& settings, const DataSet& data, GenerationStats* stats, VoicingCache* sharedCache)
    {
        const auto errors = validate (settings, data);
        if (!errors.empty ())
            throw GenerationError (joinErrors (errors));

        const auto      totalStart = Clock::now ();
        GenerationStats local;

        try
        {
            const auto factory = makeRngFactory (settings.masterSeed, settings.rerolls);
            const auto tuning  = findTuning (data, settings.tuning);

            // A non-standard tuning needs a copy of the data without the open shapes.
            DataSet        tuned;
            const DataSet* use = &data;
            if (settings.tuning != "Standard")
            {
                tuned = dataForTuning (data, settings.tuning);
                use   = &tuned;
            }

            auto start   = Clock::now ();
            Rng  planRng = factory (Stage::Plan, 0);
            auto plan    = planSong (settings, planRng, *use);
            applyPlanLocks (plan, settings);
            local.planMs = msSince (start);

            start             = Clock::now ();
            auto progressions = makeChords (plan, factory, *use);
            for (std::size_t i = 0; i < progressions.sections.size (); ++i)
                if (sectionLock (settings, i).chords)
                    lockChords (progressions.sections[i], settings.previous->sections[i], chordsPerBar (plan));
            local.chordsMs = msSince (start);

            start = Clock::now ();
            const Fretboard fretboard (tuning, plan.capo);
            VoicingCache    localCache;
            // A tuned data set is a new DataSet, so it cannot use a cache of another data set.
            VoicingCache& cache  = sharedCache != nullptr && use == &data ? *sharedCache : localCache;
            auto          voiced = voiceChords (progressions, plan.key, fretboard, plan.difficulty, settings.rerolls.voicings, cache, *use);

            // A voicing lock works only if the chords, the key, the tuning and the capo are the same.
            for (std::size_t i = 0; i < voiced.sections.size (); ++i)
            {
                if (!sectionLock (settings, i).voicings)
                    continue;
                const auto& previous = *settings.previous;
                const auto& old      = previous.sections[i];
                if (previous.key == plan.key && previous.capo == plan.capo && previous.tuning == tuning && sameChords (old.bars, voiced.sections[i].bars))
                    voiced.sections[i].bars = old.bars;
            }
            local.voicingMs = msSince (start);

            start       = Clock::now ();
            auto strums = makeStrums (plan, factory, StrumLibrary::fromData (*use));
            for (std::size_t i = 0; i < strums.size (); ++i)
                if (sectionLock (settings, i).strum)
                    strums[i] = settings.previous->sections[i].strum;
            local.strumMs = msSince (start);

            start = Clock::now ();
            std::vector<MelodyNote> hook;
            if (settings.hook)
                hook = makeHook (plan, voiced, factory, *use);
            local.hookMs = msSince (start);

            start            = Clock::now ();
            auto starter     = assemble (plan, voiced, strums, hook, tuning, settings.masterSeed);
            starter.rerolls  = settings.rerolls;
            starter.tables   = settings.tables;
            local.assembleMs = msSince (start);
            local.totalMs    = msSince (totalStart);

            if (stats != nullptr)
                *stats = local;
            return starter;
        }
        catch (const GenerationError&)
        {
            throw;
        }
        catch (const std::exception& error)
        {
            throw GenerationError (error.what ());
        }
    }

    SongStarter reroll (const SongStarter& starter, Stage stage, int sectionIndex, const DataSet& data)
    {
        if (stage == Stage::Plan || stage == Stage::Render)
            throw std::invalid_argument ("Only the Chords, Voicing, Strum and Hook stages can reroll");
        if (stage != Stage::Hook && (sectionIndex < 0 || sectionIndex >= static_cast<int> (starter.sections.size ())))
            throw std::invalid_argument ("The song starter has no section " + std::to_string (sectionIndex));

        SongStarter result = starter;
        auto&       counts = result.rerolls;
        const auto  index  = static_cast<std::size_t> (std::max (0, sectionIndex));

        const auto increase = [&counts, index] (Stage s)
        {
            auto* list = countsFor (counts, s);
            if (list->size () <= index)
                list->resize (index + 1, 0);
            ++(*list)[index];
        };

        try
        {
            const DataSet tuned   = dataForTuning (data, starter.tuning.name);
            const auto    plan    = planOf (starter);
            auto          factory = makeRngFactory (starter.seed, counts);
            VoicingCache  cache;

            switch (stage)
            {
                case Stage::Chords:
                {
                    increase (Stage::Chords);
                    factory                     = makeRngFactory (starter.seed, counts);
                    const auto progressions     = makeChords (plan, factory, tuned);
                    const auto voiced           = voiceSection (progressions.sections[index], plan.key, fretboardFor (starter), plan.difficulty, rerollCount (counts, Stage::Voicing, sectionIndex), cache, tuned);
                    result.sections[index].bars = voiced.bars;
                    break;
                }
                case Stage::Voicing:
                {
                    increase (Stage::Voicing);
                    const auto progressions     = progressionsOf (starter);
                    const auto voiced           = voiceSection (progressions.sections[index], plan.key, fretboardFor (starter), plan.difficulty, rerollCount (counts, Stage::Voicing, sectionIndex), cache, tuned);
                    result.sections[index].bars = voiced.bars;
                    break;
                }
                case Stage::Strum:
                {
                    increase (Stage::Strum);
                    factory                      = makeRngFactory (starter.seed, counts);
                    result.sections[index].strum = makeSectionStrum (plan, sectionIndex, factory, StrumLibrary::fromData (tuned));
                    break;
                }
                case Stage::Hook:
                {
                    ++counts.hook;
                    factory     = makeRngFactory (starter.seed, counts);
                    result.hook = makeHook (plan, progressionsOf (result), factory, tuned);
                    break;
                }
                case Stage::Plan:
                case Stage::Render: break;
            }
        }
        catch (const GenerationError&)
        {
            throw;
        }
        catch (const std::exception& error)
        {
            throw GenerationError (error.what ());
        }

        result.rating.reset ();
        return result;
    }

    DataSet dataForTables (const DataSet& data, const std::string& tables, const std::filesystem::path& learnedFolder, std::vector<std::string>* warnings)
    {
        if (tables != "learned")
            return data;

        const auto transitions = learnedFolder / "transitions.learned.json";
        const auto strums      = learnedFolder / "strums.learned.json";

        std::error_code error;
        const bool      hasTransitions = std::filesystem::exists (transitions, error);
        const bool      hasStrums      = std::filesystem::exists (strums, error);

        if (!hasTransitions && !hasStrums)
        {
            if (warnings != nullptr)
                warnings->push_back ("No learned tables in " + learnedFolder.string () + ". The rule tables are in use.");
            return data;
        }

        DataSet learned = data;
        if (hasTransitions)
            learned.transitions = TransitionTable::fromJsonText (readFile (transitions), "transitions.learned.json");
        if (hasStrums)
            learned.strumWeights = parseStrumWeights (readFile (strums), "strums.learned.json");
        return learned;
    }
}

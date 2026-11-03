#pragma once

#include "Chords.h"
#include "Data.h"
#include "GenerationError.h"
#include "Plan.h"
#include "Seeds.h"
#include "Settings.h"
#include "Types.h"

#include <filesystem>
#include <string>
#include <vector>

namespace algo
{
    class VoicingCache;

    // The time of each stage in milliseconds.
    struct GenerationStats
    {
        double planMs { 0.0 };
        double chordsMs { 0.0 };
        double voicingMs { 0.0 };
        double strumMs { 0.0 };
        double hookMs { 0.0 };
        double assembleMs { 0.0 };
        double totalMs { 0.0 };
    };

    // Returns the reroll count of a stage and section. A section with no value has the count 0.
    // The Plan and Render stages have no reroll count.
    int rerollCount (const RerollCounts& counts, Stage stage, int sectionIndex);

    // Makes the factory that gives each stage and section its own stream.
    RngFactory makeRngFactory (std::uint64_t masterSeed, const RerollCounts& counts);

    // Joins the stage results into one song starter.
    SongStarter assemble (const SongPlan& plan, const ProgressionSet& progressions, const std::vector<StrumPattern>& strums, const std::vector<MelodyNote>& hook, const Tuning& tuning, std::uint64_t seed);

    // Makes the plan of a song starter again from its fields, so that one stage can run again.
    SongPlan planOf (const SongStarter& starter);

    // The chords of a song starter, as the voicing and hook stages use them.
    ProgressionSet progressionsOf (const SongStarter& starter);

    // Returns the data for a tuning. A tuning that is not Standard cannot use the open shapes (T16-07).
    DataSet dataForTuning (const DataSet& data, const std::string& tuning);

    // Validates the settings and runs the stages in this order: plan, chords, voicing, strum and hook.
    // A lock copies its value from settings.previous. If previous is empty, the locks have no effect.
    // Throws GenerationError with all error texts if the settings are not valid, or if a stage fails.
    // A cache keeps the voicing candidates between calls. Use one cache for one DataSet and one thread.
    SongStarter generate (const GenerationSettings& settings, const DataSet& data = bundledData (), GenerationStats* stats = nullptr, VoicingCache* cache = nullptr);

    // Increases the reroll count of the stage and section, and runs that stage for that section only.
    // A reroll of Chords also runs the Voicing stage for the section. Stage::Hook ignores the section index.
    // Throws GenerationError if the stage fails, and std::invalid_argument for Plan, Render or an unknown section.
    SongStarter reroll (const SongStarter& starter, Stage stage, int sectionIndex, const DataSet& data = bundledData ());

    // Uses the learned tables (SPEC-15) if tables is "learned" and the folder has
    // transitions.learned.json or strums.learned.json. If no learned file exists, adds a warning and
    // returns the rule tables. Throws DataError if a learned file is not valid.
    DataSet dataForTables (const DataSet& data, const std::string& tables, const std::filesystem::path& learnedFolder, std::vector<std::string>* warnings = nullptr);
}

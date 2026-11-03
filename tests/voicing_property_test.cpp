#include "algo/Chords.h"
#include "algo/Nashville.h"
#include "algo/Pitch.h"
#include "algo/Plan.h"
#include "algo/VoicingCache.h"

#include "support/PropertyTest.h"
#include "support/VoicingHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

using namespace algo;
using algo::test::textOf;

namespace
{
    const std::vector<std::string> kForms { "Verse-Chorus", "Verse-Chorus-Bridge", "Loop" };
    const std::vector<Mood>        kMoods { Mood::Bright, Mood::Sad, Mood::Hopeful, Mood::Bluesy };

    GenerationSettings settingsFor (std::uint64_t seed, Difficulty difficulty)
    {
        GenerationSettings settings;
        settings.difficulty = difficulty;
        settings.masterSeed = seed;
        settings.mood       = kMoods[seed % kMoods.size ()];
        settings.form       = kForms[seed % kForms.size ()];
        return settings;
    }
}

TEST_CASE ("Playable voicing and correct chord: each voicing of each seed passes the filter")
{
    VoicingCache cache; // The search runs once for each chord. This makes the test fast and it does not change the result.

    algo::test::forEachSeed ([&cache] (std::uint64_t seed)
    {
        for (const auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
        {
            CAPTURE (static_cast<int> (difficulty));

            auto       planRng = rngFor (seed, Stage::Plan, -1, 0);
            const auto plan    = planSong (settingsFor (seed, difficulty), planRng);

            const RngFactory rngFactory = [seed] (Stage stage, int section)
            {
                return rngFor (seed, stage, section, 0);
            };
            const auto      progressions = makeChords (plan, rngFactory);
            const Fretboard fretboard (Tuning {}, plan.capo);

            const auto voiced = voiceChords (progressions, plan.key, fretboard, difficulty, {}, cache);

            for (const auto& section : voiced.sections)
                for (const auto& bar : section.bars)
                {
                    REQUIRE (bar.voicings.size () == bar.chords.size ());

                    for (std::size_t i = 0; i < bar.voicings.size (); ++i)
                    {
                        const auto& voicing = bar.voicings[i];
                        const auto  chord   = concreteChord (bar.chords[i], plan.key);

                        CAPTURE (chord.name, textOf (voicing));

                        // "Playable voicing": this checks REQ-07-1 to REQ-07-7.
                        REQUIRE (violations (voicing, chord, difficulty, fretboard).empty ());
                        REQUIRE (assignFingers (voicing).has_value ());
                        REQUIRE (voicing == *assignFingers (voicing));

                        // "Correct chord": this repeats the filter with no use of its code.
                        std::set<PitchClass> sounding;
                        for (int s = 0; s < kNumStrings; ++s)
                            if (voicing.frets[static_cast<std::size_t> (s)] != kMutedString)
                                sounding.insert (pitchClassOf (fretboard.midiNoteAt (s, voicing.frets[static_cast<std::size_t> (s)])));

                        std::set<PitchClass> allowed { chord.root };
                        std::set<PitchClass> required { chord.root };
                        if (chord.bass)
                        {
                            allowed.insert (*chord.bass);
                            required.insert (*chord.bass);
                        }
                        for (int interval : chord.definition.intervals)
                        {
                            allowed.insert (pitchClassOf (chord.root + interval));

                            bool isOptional = false;
                            for (int optional : chord.definition.optional)
                                isOptional = isOptional || optional == interval;
                            if (!isOptional)
                                required.insert (pitchClassOf (chord.root + interval));
                        }

                        for (PitchClass pc : sounding)
                            REQUIRE (allowed.count (pc) == 1);
                        for (PitchClass pc : required)
                            REQUIRE (sounding.count (pc) == 1);
                    }
                }
        }
    });
}

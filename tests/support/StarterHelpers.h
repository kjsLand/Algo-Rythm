#pragma once

#include "algo/Engine.h"
#include "algo/Settings.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace algo::test
{
    inline const std::vector<std::string>& testForms ()
    {
        static const std::vector<std::string> forms { "Verse-Chorus", "Verse-Chorus-Bridge", "Loop" };
        return forms;
    }

    inline const std::vector<Mood>& testMoods ()
    {
        static const std::vector<Mood> moods { Mood::Bright, Mood::Sad, Mood::Hopeful, Mood::Bluesy };
        return moods;
    }

    /** Settings that change with the seed, so a property test covers each mood, form and BPM range. */
    inline GenerationSettings settingsFor (std::uint64_t seed, Difficulty difficulty, bool hook = false)
    {
        GenerationSettings settings;
        settings.masterSeed = seed;
        settings.difficulty = difficulty;
        settings.mood       = testMoods ()[seed % testMoods ().size ()];
        settings.form       = testForms ()[(seed / 4) % testForms ().size ()];
        settings.bpmMin     = 60 + static_cast<int> (seed % 7) * 20; // 60 to 180
        settings.bpmMax     = settings.bpmMin + 40;                  // 100 to 220
        settings.hook       = hook;
        return settings;
    }

    /** The settings of the golden song starters. They have no hook, because the hook uses std::sin. */
    inline std::vector<std::pair<std::string, GenerationSettings>> goldenSettings ()
    {
        std::vector<std::pair<std::string, GenerationSettings>> list;

        GenerationSettings a;
        a.masterSeed = 81723;
        list.emplace_back ("beginner-bright", a);

        GenerationSettings b;
        b.masterSeed = 4242;
        b.mood       = Mood::Sad;
        b.form       = "Verse-Chorus-Bridge";
        list.emplace_back ("beginner-sad-vcb", b);

        GenerationSettings c;
        c.masterSeed = 7;
        c.difficulty = Difficulty::Intermediate;
        c.mood       = Mood::Hopeful;
        c.bpmMin     = 96;
        c.bpmMax     = 96;
        list.emplace_back ("intermediate-hopeful", c);

        GenerationSettings d;
        d.masterSeed = 123456789;
        d.difficulty = Difficulty::Advanced;
        d.mood       = Mood::Bluesy;
        d.keyRoot    = 9;
        list.emplace_back ("advanced-bluesy", d);

        GenerationSettings e;
        e.masterSeed = 99;
        e.difficulty = Difficulty::Advanced;
        e.form       = "Loop";
        e.bpmMin     = 150;
        e.bpmMax     = 170;
        list.emplace_back ("advanced-fast-loop", e);

        return list;
    }
}

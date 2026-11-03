#include "algo/Plan.h"

#include "support/PropertyTest.h"

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace algo;

namespace
{
    constexpr PitchClass kC = 0, kD = 2, kEb = 3, kE = 4, kF = 5, kFsharp = 6, kG = 7, kA = 9, kBb = 10;

    SongPlan planFor (const GenerationSettings& settings, std::uint64_t seed = 1)
    {
        Rng rng (seed);
        return planSong (settings, rng);
    }

    GenerationSettings beginnerWithMood (Mood mood)
    {
        GenerationSettings settings;
        settings.difficulty = Difficulty::Beginner;
        settings.mood       = mood;
        return settings;
    }

    GenerationSettings withKey (PitchClass root, const std::string& scale, Difficulty difficulty = Difficulty::Beginner)
    {
        GenerationSettings settings;
        settings.keyRoot    = root;
        settings.scale      = scale;
        settings.difficulty = difficulty;
        return settings;
    }
}

TEST_CASE ("The mood table gives the scale")
{
    REQUIRE (moodScale (Mood::Bright) == "Major");
    REQUIRE (moodScale (Mood::Sad) == "Natural Minor");
    REQUIRE (moodScale (Mood::Hopeful) == "Dorian");
    REQUIRE (moodScale (Mood::Bluesy) == "Mixolydian");
}

TEST_CASE ("A key with no scale in the settings uses the mood")
{
    for (auto mood : { Mood::Bright, Mood::Sad, Mood::Hopeful, Mood::Bluesy })
        REQUIRE (planFor (beginnerWithMood (mood)).key.scale == moodScale (mood));
}

TEST_CASE ("Each Beginner key is one of the 8 permitted keys")
{
    const std::set<std::pair<PitchClass, std::string>> permitted { { kG, "Major" }, { kC, "Major" }, { kD, "Major" }, { kA, "Major" }, { kE, "Major" }, { kA, "Natural Minor" }, { kE, "Natural Minor" }, { kD, "Natural Minor" } };

    std::set<std::pair<PitchClass, std::string>> seen;
    algo::test::forEachSeed ([&] (std::uint64_t seed)
    {
        const auto plan = planFor (beginnerWithMood (seed % 2 == 0 ? Mood::Bright : Mood::Sad), seed);
        REQUIRE (permitted.count ({ plan.key.root, plan.key.scale }) == 1);
        seen.insert ({ plan.key.root, plan.key.scale });
    });

    REQUIRE (seen == permitted);
}

TEST_CASE ("A Beginner key in a mode uses the roots of its major or minor family")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        const auto dorian = planFor (beginnerWithMood (Mood::Hopeful), seed);
        REQUIRE (std::set<PitchClass> { kA, kE, kD }.count (dorian.key.root) == 1);

        const auto mixolydian = planFor (beginnerWithMood (Mood::Bluesy), seed);
        REQUIRE (std::set<PitchClass> { kG, kC, kD, kA, kE }.count (mixolydian.key.root) == 1);
    });
}

TEST_CASE ("A key that is not Beginner uses all 12 roots with equal weight")
{
    for (auto difficulty : { Difficulty::Intermediate, Difficulty::Advanced })
    {
        GenerationSettings settings;
        settings.difficulty = difficulty;

        std::map<PitchClass, int> counts;
        for (std::uint64_t seed = 0; seed < 12000; ++seed)
            ++counts[planFor (settings, seed).key.root];

        REQUIRE (counts.size () == 12);
        for (const auto& [root, count] : counts)
        {
            INFO ("Root " << root);
            REQUIRE (count >= 800);
            REQUIRE (count <= 1200);
        }
    }
}

TEST_CASE ("A key and a scale from the settings never change")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        for (auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
        {
            GenerationSettings settings = withKey (kFsharp, "Lydian", difficulty);
            settings.mood               = Mood::Sad;

            const auto plan = planFor (settings, seed);
            REQUIRE (plan.key == Key { kFsharp, "Lydian" });
        }
    });
}

TEST_CASE ("A Beginner key with no open shape gets a capo suggestion")
{
    REQUIRE (planFor (withKey (kF, "Major")).capo == 1);              // An E shape
    REQUIRE (planFor (withKey (kBb, "Major")).capo == 1);             // An A shape
    REQUIRE (planFor (withKey (kEb, "Major")).capo == 1);             // A D shape
    REQUIRE (planFor (withKey (kFsharp, "Major")).capo == 2);         // An E shape
    REQUIRE (planFor (withKey (kBb, "Natural Minor")).capo == 1);     // An Am shape
    REQUIRE (planFor (withKey (kFsharp, "Natural Minor")).capo == 2); // An Em shape
}

TEST_CASE ("A Beginner key with an open shape gets no capo")
{
    for (PitchClass root : { kG, kC, kD, kA, kE })
        REQUIRE (planFor (withKey (root, "Major")).capo == 0);

    for (PitchClass root : { kA, kE, kD })
        REQUIRE (planFor (withKey (root, "Natural Minor")).capo == 0);
}

TEST_CASE ("A random Beginner key needs no capo")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        REQUIRE (planFor (beginnerWithMood (Mood::Hopeful), seed).capo == 0);
    });
}

TEST_CASE ("Only a Beginner key gets a capo suggestion")
{
    REQUIRE (planFor (withKey (kF, "Major", Difficulty::Intermediate)).capo == 0);
    REQUIRE (planFor (withKey (kF, "Major", Difficulty::Advanced)).capo == 0);
}

TEST_CASE ("A capo from the settings stays")
{
    GenerationSettings settings = withKey (kF, "Major");
    settings.capo               = 5;
    REQUIRE (planFor (settings).capo == 5);

    settings      = withKey (kC, "Major", Difficulty::Advanced);
    settings.capo = 3;
    REQUIRE (planFor (settings).capo == 3);
}

TEST_CASE ("A tonic chord with no open shape at any capo gets no capo")
{
    REQUIRE (planFor (withKey (kC, "Locrian")).capo == 0);
}

TEST_CASE ("The BPM is always even and in the range")
{
    const std::vector<std::pair<int, int>> ranges { { 90, 120 }, { 91, 99 }, { 40, 41 }, { 41, 42 }, { 219, 220 }, { 96, 96 }, { 40, 220 } };

    for (const auto& [lo, hi] : ranges)
    {
        GenerationSettings settings;
        settings.bpmMin = lo;
        settings.bpmMax = hi;

        algo::test::forEachSeed ([&, lo = lo, hi = hi] (std::uint64_t seed)
        {
            const int bpm = planFor (settings, seed).bpm;
            REQUIRE (bpm % 2 == 0);
            REQUIRE (bpm >= lo);
            REQUIRE (bpm <= hi);
        });
    }
}

TEST_CASE ("The BPM uses all even values in the range")
{
    GenerationSettings settings;
    settings.bpmMin = 91;
    settings.bpmMax = 97;

    std::set<int> seen;
    for (std::uint64_t seed = 0; seed < 1000; ++seed)
        seen.insert (planFor (settings, seed).bpm);

    REQUIRE (seen == std::set<int> { 92, 94, 96 });
}

TEST_CASE ("The form gives the sections and their bar counts")
{
    GenerationSettings settings;

    settings.form  = "Verse-Chorus-Bridge";
    const auto vcb = planFor (settings);
    REQUIRE (vcb.form == "Verse-Chorus-Bridge");
    REQUIRE (vcb.sections.size () == 3);
    REQUIRE (vcb.sections[0].name == "Verse");
    REQUIRE (vcb.sections[0].bars == 8);
    REQUIRE (vcb.sections[1].name == "Chorus");
    REQUIRE (vcb.sections[1].bars == 8);
    REQUIRE (vcb.sections[2].name == "Bridge");
    REQUIRE (vcb.sections[2].role == "bridge");
    REQUIRE (vcb.sections[2].bars == 4);

    settings.form = "Verse-Chorus";
    const auto vc = planFor (settings);
    REQUIRE (vc.sections.size () == 2);
    REQUIRE (vc.sections[0].bars == 8);
    REQUIRE (vc.sections[1].bars == 8);

    settings.form   = "Loop";
    const auto loop = planFor (settings);
    REQUIRE (loop.sections.size () == 1);
    REQUIRE (loop.sections[0].name == "Loop");
    REQUIRE (loop.sections[0].bars == 4);
}

TEST_CASE ("The plan copies the difficulty and the time signature")
{
    GenerationSettings settings;
    settings.difficulty = Difficulty::Advanced;

    const auto plan = planFor (settings);

    REQUIRE (plan.difficulty == Difficulty::Advanced);
    REQUIRE (plan.timeSig == TimeSig { 4, 4 });
}

TEST_CASE ("The same seed gives the same plan")
{
    GenerationSettings settings;
    settings.difficulty = Difficulty::Intermediate;

    const auto a = planFor (settings, 99);
    const auto b = planFor (settings, 99);

    REQUIRE (a.key == b.key);
    REQUIRE (a.bpm == b.bpm);
    REQUIRE (a.capo == b.capo);
}

TEST_CASE ("planSong throws if the settings are not valid")
{
    GenerationSettings settings;

    settings.form = "Fugue";
    REQUIRE_THROWS_AS (planFor (settings), std::invalid_argument);

    settings        = GenerationSettings {};
    settings.bpmMin = 91;
    settings.bpmMax = 91;
    REQUIRE_THROWS_AS (planFor (settings), std::invalid_argument);

    settings       = GenerationSettings {};
    settings.scale = "Blues";
    REQUIRE_THROWS_AS (planFor (settings), std::invalid_argument);
}

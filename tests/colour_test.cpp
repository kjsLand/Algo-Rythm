#include "algo/Chords.h"
#include "algo/ColourRules.h"
#include "algo/Nashville.h"

#include "support/PropertyTest.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

using namespace algo;

namespace
{
    const std::vector<Mood>        kMoods { Mood::Bright, Mood::Sad, Mood::Hopeful, Mood::Bluesy };
    const std::vector<std::string> kForms { "Verse-Chorus", "Verse-Chorus-Bridge", "Loop" };

    SongPlan planIn (const std::string& scale, Difficulty difficulty)
    {
        SongPlan plan;
        plan.key        = { 0, scale };
        plan.difficulty = difficulty;
        return plan;
    }

    ChordLoop loopOf (const std::vector<std::string>& numerals)
    {
        ChordLoop loop;
        for (const auto& numeral : numerals)
            loop.push_back (parseNumeral (numeral));
        return loop;
    }

    // Only the rule under test can change a chord.
    ColourRules noColour ()
    {
        ColourRules rules;
        rules.colourChance    = 0.0;
        rules.borrowedChance  = 0.0;
        rules.seventhChance   = 0.0;
        rules.secondaryChance = 0.0;
        return rules;
    }

    bool hasTag (const ChordSymbol& chord, const std::string& tag)
    {
        return std::find (chord.tags.begin (), chord.tags.end (), tag) != chord.tags.end ();
    }

    ProgressionSet chordsFor (std::uint64_t seed, Difficulty difficulty, SongPlan* planOut = nullptr)
    {
        GenerationSettings settings;
        settings.difficulty = difficulty;
        settings.mood       = kMoods[seed % kMoods.size ()];
        settings.form       = kForms[(seed / kMoods.size ()) % kForms.size ()];

        Rng        planRng = rngFor (seed, Stage::Plan, 0, 0);
        const auto plan    = planSong (settings, planRng);
        if (planOut != nullptr)
            *planOut = plan;

        return makeChords (plan, [&] (Stage stage, int section)
        {
            return rngFor (seed, stage, section, 0);
        });
    }
}

TEST_CASE ("The default colour rules have the values of the spec")
{
    const ColourRules rules;

    REQUIRE (rules.maxColourChords == 2);
    REQUIRE (rules.colourChance == 0.25);
    REQUIRE (rules.colourSymbols == std::vector<std::string> { "sus2", "sus4", "add9" });
    REQUIRE (rules.borrowedChance == 0.15);
    REQUIRE (rules.borrowedNumerals == std::vector<std::string> { "b7", "4m", "b6" });
    REQUIRE (rules.seventhChance == 0.4);
    REQUIRE (rules.seventhDegrees == std::vector<int> { 2, 4, 5 });
    REQUIRE (rules.secondaryChance == 0.1);
    REQUIRE (rules.secondaryNumeral == "37");
    REQUIRE (rules.secondaryTarget == 6);
}

TEST_CASE ("Beginner has no colour chords")
{
    const auto loop = loopOf ({ "1", "2m", "4", "5" });

    ColourRules always;
    always.colourChance    = 1.0;
    always.borrowedChance  = 1.0;
    always.seventhChance   = 1.0;
    always.secondaryChance = 1.0;

    Rng rng (1);
    REQUIRE (applyColour (loop, planIn ("Major", Difficulty::Beginner), rng, bundledData (), always) == loop);
}

TEST_CASE ("Each Beginner chord is a major or minor triad")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        SongPlan   plan;
        const auto chords = chordsFor (seed, Difficulty::Beginner, &plan);

        for (const auto& section : chords.sections)
        {
            for (const auto& bar : section.bars)
            {
                for (const auto& chord : bar.chords)
                {
                    const auto quality = resolve (chord, plan.key).quality;
                    REQUIRE ((quality == "" || quality == "m"));
                }
            }
        }
    });
}

TEST_CASE ("Each chord that is not diatonic has a tag")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        for (auto difficulty : { Difficulty::Intermediate, Difficulty::Advanced })
        {
            SongPlan   plan;
            const auto chords = chordsFor (seed, difficulty, &plan);

            for (const auto& section : chords.sections)
            {
                for (const auto& chord : section.loop)
                {
                    INFO (numeralText (chord) << " in " << plan.key.scale);
                    REQUIRE ((isDiatonic (chord, plan.key) || hasTag (chord, "borrowed") || hasTag (chord, "secondary")));
                }
            }
        }
    });
}

TEST_CASE ("Intermediate changes up to 2 chords to colour chords")
{
    const auto  plan   = planIn ("Major", Difficulty::Intermediate);
    const auto  loop   = loopOf ({ "1", "2m", "4", "5" });
    ColourRules rules  = noColour ();
    rules.colourChance = 1.0;

    algo::test::forEachSeed ([&] (std::uint64_t seed)
    {
        Rng        rng (seed);
        const auto coloured = applyColour (loop, plan, rng, bundledData (), rules);

        int changed = 0;
        for (std::size_t i = 0; i < loop.size (); ++i)
        {
            REQUIRE (coloured[i].degree == loop[i].degree);
            if (coloured[i].quality != loop[i].quality)
            {
                ++changed;
                REQUIRE ((coloured[i].quality == "sus2" || coloured[i].quality == "sus4" || coloured[i].quality == "add9"));
                REQUIRE (isDiatonic (coloured[i], plan.key));
                REQUIRE (coloured[i].tags.empty ());
            }
        }
        REQUIRE (changed == 2);
    });
}

TEST_CASE ("A colour chord stays in the scale")
{
    const auto  plan      = planIn ("Major", Difficulty::Intermediate);
    ColourRules rules     = noColour ();
    rules.colourChance    = 1.0;
    rules.maxColourChords = 4;

    std::set<std::string> seen;
    for (std::uint64_t seed = 0; seed < 1000; ++seed)
    {
        Rng rng (seed);

        // On a C major scale, F has no sus4 and D minor has no add9.
        for (const auto& chord : applyColour (loopOf ({ "1", "2m", "4", "5" }), plan, rng, bundledData (), rules))
            seen.insert (numeralText (chord));
    }

    REQUIRE (seen == std::set<std::string> { "1sus2", "1sus4", "1add9", "2sus2", "2sus4", "4sus2", "4add9", "5sus2", "5sus4", "5add9" });
}

TEST_CASE ("A colour chord changes only a plain triad")
{
    const auto  plan   = planIn ("Major", Difficulty::Intermediate);
    const auto  loop   = loopOf ({ "1maj7", "5/7", "b6", "57" });
    ColourRules rules  = noColour ();
    rules.colourChance = 1.0;

    ChordLoop tagged = loop;
    tagged[2].tags   = { "borrowed" };

    Rng rng (1);
    REQUIRE (applyColour (tagged, plan, rng, bundledData (), rules) == tagged);
}

TEST_CASE ("A colour chance of 0.25 changes about one chord in 4")
{
    const auto  plan      = planIn ("Major", Difficulty::Intermediate);
    const auto  loop      = loopOf ({ "1", "1", "1", "1" });
    ColourRules rules     = noColour ();
    rules.colourChance    = 0.25;
    rules.maxColourChords = 4;

    int changed = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed)
    {
        Rng        rng (seed);
        const auto coloured = applyColour (loop, plan, rng, bundledData (), rules);
        changed += static_cast<int> (std::count_if (coloured.begin (), coloured.end (), [] (const ChordSymbol& chord)
        {
            return chord.quality != kDiatonicQuality;
        }));
    }

    REQUIRE (changed >= 800);
    REQUIRE (changed <= 1200);
}

TEST_CASE ("Intermediate can add one borrowed chord")
{
    const auto  plan     = planIn ("Major", Difficulty::Intermediate);
    const auto  loop     = loopOf ({ "1", "2m", "3m", "5" });
    ColourRules rules    = noColour ();
    rules.borrowedChance = 1.0;

    std::set<std::string> seen;
    algo::test::forEachSeed ([&] (std::uint64_t seed)
    {
        Rng        rng (seed);
        const auto coloured = applyColour (loop, plan, rng, bundledData (), rules);

        REQUIRE (coloured.size () == 4);
        REQUIRE (coloured.front () == loop.front ());
        REQUIRE (coloured.back () == loop.back ());

        int borrowed = 0;
        for (const auto& chord : coloured)
        {
            if (hasTag (chord, "borrowed"))
            {
                ++borrowed;
                seen.insert (numeralText (chord));
                REQUIRE_FALSE (isDiatonic (chord, plan.key));
            }
        }
        REQUIRE (borrowed == 1);
    });

    REQUIRE (seen == std::set<std::string> { "b7", "4m", "b6" });
}

TEST_CASE ("A loop with a borrowed chord gets no second borrowed chord")
{
    const auto  plan     = planIn ("Major", Difficulty::Intermediate);
    ChordLoop   loop     = loopOf ({ "1", "b7", "4", "5" });
    ColourRules rules    = noColour ();
    rules.borrowedChance = 1.0;
    loop[1].tags         = { "borrowed" };

    Rng rng (1);
    REQUIRE (applyColour (loop, plan, rng, bundledData (), rules) == loop);
}

TEST_CASE ("A borrowed chord does not make a direct repeat")
{
    const auto  plan     = planIn ("Major", Difficulty::Intermediate);
    const auto  loop     = loopOf ({ "1", "4", "2m", "4" });
    ColourRules rules    = noColour ();
    rules.borrowedChance = 1.0;

    algo::test::forEachSeed ([&] (std::uint64_t seed)
    {
        Rng        rng (seed);
        const auto coloured = applyColour (loop, plan, rng, bundledData (), rules);

        for (std::size_t i = 1; i < coloured.size (); ++i)
            REQUIRE_FALSE ((coloured[i].degree == coloured[i - 1].degree && coloured[i].accidental == coloured[i - 1].accidental));
    });
}

TEST_CASE ("A numeral that is diatonic or has a different root in a key is not borrowed")
{
    ColourRules rules    = noColour ();
    rules.borrowedChance = 1.0;

    // In Natural Minor, 4m is diatonic, and b7 and b6 are a semitone below the borrowed chords.
    const auto minor     = planIn ("Natural Minor", Difficulty::Intermediate);
    const auto minorLoop = loopOf ({ "1m", "3", "6", "7" });
    for (std::uint64_t seed = 0; seed < 100; ++seed)
    {
        Rng rng (seed);
        REQUIRE (applyColour (minorLoop, minor, rng, bundledData (), rules) == minorLoop);
    }

    // In Mixolydian, 7 is the chord b7. The numeral b7 is not the same chord.
    const auto mixolydian = planIn ("Mixolydian", Difficulty::Intermediate);
    const auto mixoLoop   = loopOf ({ "1", "2m", "3dim", "4" });
    for (std::uint64_t seed = 0; seed < 100; ++seed)
    {
        Rng        rng (seed);
        const auto coloured = applyColour (mixoLoop, mixolydian, rng, bundledData (), rules);

        for (const auto& chord : coloured)
            REQUIRE_FALSE ((chord.degree == 7 && chord.accidental == -1));
    }
}

TEST_CASE ("Advanced changes 2, 4 and 5 to 7th chords")
{
    const auto  plan    = planIn ("Major", Difficulty::Advanced);
    ColourRules rules   = noColour ();
    rules.seventhChance = 1.0;

    Rng        rng (1);
    const auto coloured = applyColour (loopOf ({ "1", "2m", "4", "5" }), plan, rng, bundledData (), rules);

    REQUIRE (numeralText (coloured[0]) == "1");
    REQUIRE (numeralText (coloured[1]) == "2m7");
    REQUIRE (numeralText (coloured[2]) == "4maj7");
    REQUIRE (numeralText (coloured[3]) == "57");
}

TEST_CASE ("A 7th chord is the diatonic 7th chord of the key")
{
    ColourRules rules   = noColour ();
    rules.seventhChance = 1.0;

    Rng rng (1);

    const auto minor = applyColour (loopOf ({ "2", "4", "5", "1" }), planIn ("Natural Minor", Difficulty::Advanced), rng, bundledData (), rules);
    REQUIRE (minor[0].quality == kDiatonicQuality); // A diminished chord is not a plain triad
    REQUIRE (minor[1].quality == "m7");
    REQUIRE (minor[2].quality == "m7");

    const auto mixolydian = applyColour (loopOf ({ "2", "4", "5", "1" }), planIn ("Mixolydian", Difficulty::Advanced), rng, bundledData (), rules);
    REQUIRE (mixolydian[0].quality == "m7");
    REQUIRE (mixolydian[1].quality == "maj7");
    REQUIRE (mixolydian[2].quality == "m7");
}

TEST_CASE ("A 7th chance of 0.4 changes about 2 chords in 5")
{
    const auto  plan    = planIn ("Major", Difficulty::Advanced);
    const auto  loop    = loopOf ({ "2", "4", "5", "4" });
    ColourRules rules   = noColour ();
    rules.seventhChance = 0.4;

    int changed = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed)
    {
        Rng        rng (seed);
        const auto coloured = applyColour (loop, plan, rng, bundledData (), rules);
        changed += static_cast<int> (std::count_if (coloured.begin (), coloured.end (), [] (const ChordSymbol& chord)
        {
            return chord.quality != kDiatonicQuality;
        }));
    }

    REQUIRE (changed >= 1400);
    REQUIRE (changed <= 1800);
}

TEST_CASE ("Advanced can add the secondary dominant of 6")
{
    const auto  plan      = planIn ("Major", Difficulty::Advanced);
    ColourRules rules     = noColour ();
    rules.secondaryChance = 1.0;

    Rng        rng (1);
    const auto coloured = applyColour (loopOf ({ "1", "2m", "6m", "4" }), plan, rng, bundledData (), rules);

    REQUIRE (numeralText (coloured[0]) == "1");
    REQUIRE (numeralText (coloured[1]) == "37");
    REQUIRE (coloured[1].tags == std::vector<std::string> { "secondary" });
    REQUIRE (numeralText (coloured[2]) == "6m");
    REQUIRE (numeralText (coloured[3]) == "4");
}

TEST_CASE ("A loop with no 6 after a middle chord gets no secondary dominant")
{
    const auto  plan      = planIn ("Major", Difficulty::Advanced);
    ColourRules rules     = noColour ();
    rules.secondaryChance = 1.0;

    for (const auto& loop : { loopOf ({ "1", "2m", "4", "5" }), loopOf ({ "6m", "2m", "4", "5" }) })
    {
        Rng rng (1);
        REQUIRE (applyColour (loop, plan, rng, bundledData (), rules) == loop);
    }
}

TEST_CASE ("A secondary dominant needs a target a fifth below it")
{
    ColourRules rules     = noColour ();
    rules.secondaryChance = 1.0;

    // In Dorian, 3 is a tritone from 6.
    const auto loop = loopOf ({ "1m", "2m", "6dim", "4" });
    Rng        rng (1);
    REQUIRE (applyColour (loop, planIn ("Dorian", Difficulty::Advanced), rng, bundledData (), rules) == loop);

    // In Natural Minor, C7 goes to F in the key of A.
    const auto coloured = applyColour (loopOf ({ "1m", "4m", "6", "5m" }), planIn ("Natural Minor", Difficulty::Advanced), rng, bundledData (), rules);
    REQUIRE (numeralText (coloured[1]) == "37");
}

TEST_CASE ("The colour rules of one difficulty do not apply to another difficulty")
{
    const auto loop = loopOf ({ "1", "2m", "6m", "5" });

    ColourRules advancedOnly     = noColour ();
    advancedOnly.seventhChance   = 1.0;
    advancedOnly.secondaryChance = 1.0;
    Rng rng (1);
    REQUIRE (applyColour (loop, planIn ("Major", Difficulty::Intermediate), rng, bundledData (), advancedOnly) == loop);

    ColourRules intermediateOnly    = noColour ();
    intermediateOnly.colourChance   = 1.0;
    intermediateOnly.borrowedChance = 1.0;
    REQUIRE (applyColour (loop, planIn ("Major", Difficulty::Advanced), rng, bundledData (), intermediateOnly) == loop);
}

TEST_CASE ("Colour chords appear in generated songs")
{
    int coloured  = 0;
    int borrowed  = 0;
    int sevenths  = 0;
    int secondary = 0;

    for (std::uint64_t seed = 0; seed < 1000; ++seed)
    {
        for (const auto& section : chordsFor (seed, Difficulty::Intermediate).sections)
        {
            for (const auto& chord : section.loop)
            {
                coloured += chord.quality == "sus2" || chord.quality == "sus4" || chord.quality == "add9" ? 1 : 0;
                borrowed += hasTag (chord, "borrowed") ? 1 : 0;
            }
        }
        for (const auto& section : chordsFor (seed, Difficulty::Advanced).sections)
        {
            for (const auto& chord : section.loop)
            {
                sevenths += chord.quality == "m7" || chord.quality == "maj7" || chord.quality == "7" ? 1 : 0;
                secondary += hasTag (chord, "secondary") ? 1 : 0;
            }
        }
    }

    REQUIRE (coloured > 0);
    REQUIRE (borrowed > 0);
    REQUIRE (sevenths > 0);
    REQUIRE (secondary > 0);
}

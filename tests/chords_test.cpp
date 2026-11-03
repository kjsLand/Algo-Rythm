#include "algo/Chords.h"
#include "algo/DataError.h"
#include "algo/Nashville.h"

#include "support/PropertyTest.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace algo;
using Catch::Matchers::ContainsSubstring;

namespace
{
    const std::vector<std::string> kRoles { "verse", "chorus", "bridge", "loop" };
    const std::vector<Difficulty>  kDifficulties { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced };
    const std::vector<std::string> kForms { "Verse-Chorus", "Verse-Chorus-Bridge", "Loop" };
    const std::vector<Mood>        kMoods { Mood::Bright, Mood::Sad, Mood::Hopeful, Mood::Bluesy };

    SongPlan planIn (const std::string& scale, Difficulty difficulty = Difficulty::Beginner, PitchClass root = 0)
    {
        SongPlan plan;
        plan.key        = { root, scale };
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

    std::vector<std::string> rootsOf (const ChordLoop& loop)
    {
        std::vector<std::string> roots;
        for (auto chord : loop)
        {
            chord.quality    = "diatonic";
            chord.bassDegree = 0;
            chord.tags.clear ();
            roots.push_back (numeralText (chord));
        }
        return roots;
    }

    bool hasFailure (const std::vector<std::string>& failures, const std::string& text)
    {
        return std::any_of (failures.begin (), failures.end (), [&] (const std::string& failure)
        {
            return failure.find (text) != std::string::npos;
        });
    }

    bool hasTag (const ChordSymbol& chord)
    {
        return !chord.tags.empty ();
    }

    bool isLibraryLoop (const ChordLoop& loop, const std::string& mode, const DataSet& data)
    {
        const auto progressions = data.progressions.filter ({ mode, "", "", std::nullopt });
        return std::any_of (progressions.begin (), progressions.end (), [&] (const ProgressionDefinition& progression)
        {
            return loopOf (progression.numerals) == loop;
        });
    }

    // One song for a seed. The seed also selects the mood and the form.
    struct Song
    {
        SongPlan       plan;
        ProgressionSet chords;
    };

    Song songFor (std::uint64_t seed, Difficulty difficulty, const std::vector<int>& rerolls = {})
    {
        GenerationSettings settings;
        settings.difficulty = difficulty;
        settings.mood       = kMoods[seed % kMoods.size ()];
        settings.form       = kForms[(seed / kMoods.size ()) % kForms.size ()];
        settings.bpmMin     = 60;
        settings.bpmMax     = 140;

        Rng  planRng = rngFor (seed, Stage::Plan, 0, 0);
        Song song;
        song.plan   = planSong (settings, planRng);
        song.chords = makeChords (song.plan, [&] (Stage stage, int section)
        {
            const auto index = static_cast<std::size_t> (section);
            return rngFor (seed, stage, section, index < rerolls.size () ? rerolls[index] : 0);
        });
        return song;
    }

    const std::vector<ChordLoop> kNoOtherLoops;
}

TEST_CASE ("A Beginner chorus in a major key always gets a progression with the role chorus")
{
    const auto plan = planIn ("Major");

    algo::test::forEachSeed ([&] (std::uint64_t seed)
    {
        Rng        rng (seed);
        const auto progression = pickLibraryLoop (plan, "chorus", Difficulty::Beginner, rng);

        REQUIRE (progression.mode == "Major");
        REQUIRE (progression.difficulty == Difficulty::Beginner);
        REQUIRE (std::find (progression.roles.begin (), progression.roles.end (), "chorus") != progression.roles.end ());
    });
}

TEST_CASE ("pickLibraryLoop uses all progressions that pass the filter")
{
    const auto                  plan = planIn ("Major");
    const std::set<std::string> permitted { "Axis", "Minor Axis", "Fifties", "Three Chord Rock", "Rock Anthem", "Pop Climb", "Folk Swing", "Ballad Rise" };

    std::set<std::string> seen;
    for (std::uint64_t seed = 0; seed < 1000; ++seed)
    {
        Rng rng (seed);
        seen.insert (pickLibraryLoop (plan, "chorus", Difficulty::Beginner, rng).name);
    }

    REQUIRE (seen == permitted);
}

TEST_CASE ("pickLibraryLoop removes the role filter if no progression passes")
{
    DataSet data      = loadBundledData ();
    data.progressions = ProgressionLibrary::fromJsonText (R"({ "schemaVersion": 1, "progressions": [
        { "name": "Only", "numerals": ["1", "4", "1", "5"], "mode": "Major", "tags": [], "roles": ["verse"], "difficulty": "Beginner" } ] })",
                                                          data.scales, data.chords);

    Rng rng (1);
    REQUIRE (pickLibraryLoop (planIn ("Major"), "bridge", Difficulty::Beginner, rng, data).name == "Only");
}

TEST_CASE ("pickLibraryLoop uses the major or minor family for a scale with no progressions")
{
    Rng rng (1);

    REQUIRE (pickLibraryLoop (planIn ("Lydian"), "verse", Difficulty::Beginner, rng).mode == "Major");
    REQUIRE (pickLibraryLoop (planIn ("Phrygian"), "verse", Difficulty::Beginner, rng).mode == "Natural Minor");
}

TEST_CASE ("pickLibraryLoop throws if the library has no progression to use")
{
    DataSet data      = loadBundledData ();
    data.progressions = ProgressionLibrary::fromJsonText (R"({ "schemaVersion": 1, "progressions": [] })", data.scales, data.chords);

    Rng rng (1);
    REQUIRE_THROWS_AS (pickLibraryLoop (planIn ("Major"), "verse", Difficulty::Beginner, rng, data), DataError);
}

TEST_CASE ("libraryLoop tags only the chords that are not diatonic")
{
    const auto& library = bundledData ().progressions.all ();
    const auto  find    = [&] (const std::string& name)
    {
        return *std::find_if (library.begin (), library.end (), [&] (const ProgressionDefinition& progression)
        {
            return progression.name == name;
        });
    };
    const Key cMajor { 0, "Major" };

    const auto borrowed = libraryLoop (find ("Flat Seven Rock"), cMajor);
    REQUIRE (borrowed[0].tags.empty ());
    REQUIRE (borrowed[1].tags == std::vector<std::string> { "borrowed" });
    REQUIRE (borrowed[2].tags.empty ());
    REQUIRE (borrowed[3].tags == std::vector<std::string> { "borrowed" });

    const auto secondary = libraryLoop (find ("Secondary Lift"), cMajor);
    REQUIRE (secondary[0].tags.empty ());
    REQUIRE (secondary[1].tags == std::vector<std::string> { "secondary" });

    const auto diatonic = libraryLoop (find ("Axis"), cMajor);
    REQUIRE (std::none_of (diatonic.begin (), diatonic.end (), hasTag));
}

TEST_CASE ("libraryLoop keeps the chord types of a progression from another mode")
{
    const auto& library = bundledData ().progressions.all ();
    const auto  axis    = *std::find_if (library.begin (), library.end (), [] (const ProgressionDefinition& progression)
        {
        return progression.name == "Axis";
    });
    const Key   cLydian { 0, "Lydian" };

    // The chord on degree 4 of Lydian is diminished. The Major progression has a major chord there.
    const auto loop = libraryLoop (axis, cLydian);
    REQUIRE (loop[3].degree == 4);
    REQUIRE (resolve (loop[3], cLydian).quality == "");
    REQUIRE (loop[3].tags == std::vector<std::string> { "borrowed" });
}

TEST_CASE ("Each bundled progression passes the rules for its roles")
{
    const auto& data = bundledData ();
    for (const auto& progression : data.progressions.all ())
    {
        const auto plan = planIn (progression.mode, progression.difficulty);
        const auto loop = libraryLoop (progression, plan.key, data);

        for (const auto& role : progression.roles)
        {
            INFO (progression.name << " as " << role);
            REQUIRE (checkLoop (loop, role, kNoOtherLoops, plan, data).empty ());
        }
    }
}

TEST_CASE ("startNumerals gives the start numerals for each role")
{
    REQUIRE (startNumerals ("verse") == std::vector<int> { 1 });
    REQUIRE (startNumerals ("chorus") == std::vector<int> { 4, 6, 1 });
    REQUIRE (startNumerals ("bridge") == std::vector<int> { 4, 6, 2 });
    REQUIRE (startNumerals ("loop") == std::vector<int> { 1 });
    REQUIRE_THROWS_AS (startNumerals ("intro"), std::invalid_argument);
}

TEST_CASE ("Each walk has 4 numerals with no direct repeats")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        for (const char* scale : { "Major", "Natural Minor", "Dorian", "Mixolydian" })
        {
            const auto plan = planIn (scale, kDifficulties[seed % kDifficulties.size ()]);
            for (const auto& role : kRoles)
            {
                for (int start : startNumerals (role))
                {
                    Rng        rng (seed);
                    const auto loop = walkLoop (plan, role, start, rng);

                    // A Beginner cannot start on a diminished chord, for example 2 in Natural Minor.
                    ChordSymbol first;
                    first.degree = start;
                    if (plan.difficulty == Difficulty::Beginner && resolve (first, plan.key).quality == "dim")
                    {
                        REQUIRE (loop.empty ());
                        continue;
                    }

                    REQUIRE (loop.size () == 4);
                    REQUIRE (loop[0].degree == start);
                    for (std::size_t i = 1; i < loop.size (); ++i)
                        REQUIRE (loop[i].degree != loop[i - 1].degree);
                }
            }
        }
    });
}

TEST_CASE ("Each verse walk starts on 1")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        Rng        rng (seed);
        const auto loop = walkLoop (planIn ("Major"), "verse", startNumerals ("verse")[0], rng);

        REQUIRE (loop[0].degree == 1);
    });
}

TEST_CASE ("A walk cannot start on a numeral that its role does not permit")
{
    Rng rng (1);

    REQUIRE_THROWS_AS (walkLoop (planIn ("Major"), "verse", 4, rng), std::invalid_argument);
    REQUIRE_THROWS_AS (walkLoop (planIn ("Major"), "bridge", 1, rng), std::invalid_argument);
    REQUIRE_THROWS_AS (walkLoop (planIn ("Major"), "intro", 1, rng), std::invalid_argument);
}

TEST_CASE ("A Beginner walk has only major and minor triads")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        for (const char* scale : { "Major", "Natural Minor", "Dorian", "Mixolydian", "Harmonic Minor" })
        {
            const auto plan = planIn (scale);
            Rng        rng (seed);

            for (const auto& chord : walkLoop (plan, "verse", 1, rng))
            {
                const auto quality = resolve (chord, plan.key).quality;
                REQUIRE ((quality == "" || quality == "m"));
            }
        }
    });
}

TEST_CASE ("A walk that is not Beginner can use a diminished chord")
{
    const auto plan = planIn ("Major", Difficulty::Intermediate);

    std::set<int> seen;
    for (std::uint64_t seed = 0; seed < 1000; ++seed)
    {
        Rng rng (seed);
        for (const auto& chord : walkLoop (plan, "verse", 1, rng))
            seen.insert (chord.degree);
    }

    REQUIRE (seen.count (7) == 1);
}

TEST_CASE ("A Beginner walk from a diminished chord gives an empty loop")
{
    Rng rng (1);

    REQUIRE (walkLoop (planIn ("Locrian"), "verse", 1, rng).empty ());
}

TEST_CASE ("checkLoop permits only the listed changes back to the first chord")
{
    const auto plan = planIn ("Major", Difficulty::Advanced);

    for (const auto& numerals : std::vector<std::vector<std::string>> { { "1", "4", "6m", "5" }, { "1", "5", "6m", "4" }, { "1", "4", "5", "b7" }, { "6m", "1", "5", "4" }, { "6m", "1", "4", "5" } })
    {
        ChordLoop loop = loopOf (numerals);
        loop[3].tags   = numerals[3] == "b7" ? std::vector<std::string> { "borrowed" } : std::vector<std::string> {};

        INFO (numerals[3] << " to " << numerals[0]);
        REQUIRE (checkLoop (loop, "verse", kNoOtherLoops, plan).empty ());
    }

    for (const auto& numerals : std::vector<std::vector<std::string>> { { "1", "5", "4", "6m" }, { "1", "4", "5", "2m" }, { "1", "4", "5", "7dim" }, { "6m", "1", "4", "2m" }, { "4", "1", "6m", "5" }, { "2m", "4", "6m", "5" } })
    {
        INFO (numerals[3] << " to " << numerals[0]);
        REQUIRE (hasFailure (checkLoop (loopOf (numerals), "verse", kNoOtherLoops, plan), "does not lead back"));
    }
}

TEST_CASE ("checkLoop treats the numeral 7 of a scale with a flat 7th as b7")
{
    REQUIRE (checkLoop (loopOf ({ "1m", "6", "3", "7" }), "verse", kNoOtherLoops, planIn ("Natural Minor")).empty ());
    REQUIRE (checkLoop (loopOf ({ "1", "4", "5m", "7" }), "verse", kNoOtherLoops, planIn ("Mixolydian")).empty ());

    // In these keys the root of the chord is not a b7.
    REQUIRE (hasFailure (checkLoop (loopOf ({ "1", "4", "5", "7dim" }), "verse", kNoOtherLoops, planIn ("Major", Difficulty::Advanced)), "does not lead back"));

    ChordLoop flat = loopOf ({ "1m", "4m", "5m", "b7" });
    flat[3].tags   = { "borrowed" };
    REQUIRE (hasFailure (checkLoop (flat, "verse", kNoOtherLoops, planIn ("Natural Minor")), "does not lead back"));
}

TEST_CASE ("checkLoop needs 4 chords")
{
    const auto plan = planIn ("Major");

    REQUIRE (hasFailure (checkLoop ({}, "verse", kNoOtherLoops, plan), "4 chords"));
    REQUIRE (hasFailure (checkLoop (loopOf ({ "1", "5" }), "verse", kNoOtherLoops, plan), "4 chords: 2"));
    REQUIRE (hasFailure (checkLoop (loopOf ({ "1", "4", "1", "4", "5" }), "verse", kNoOtherLoops, plan), "4 chords: 5"));
}

TEST_CASE ("checkLoop needs a bridge to end on 5")
{
    const auto plan = planIn ("Major");

    REQUIRE (checkLoop (loopOf ({ "6m", "4", "1", "5" }), "bridge", kNoOtherLoops, plan).empty ());
    REQUIRE (hasFailure (checkLoop (loopOf ({ "1", "5", "6m", "4" }), "bridge", kNoOtherLoops, plan), "bridge must end on 5"));
    REQUIRE (checkLoop (loopOf ({ "1", "5", "6m", "4" }), "verse", kNoOtherLoops, plan).empty ());
}

TEST_CASE ("checkLoop needs the chorus to differ from the verse")
{
    const auto plan  = planIn ("Major", Difficulty::Advanced);
    const auto verse = loopOf ({ "1", "5", "6m", "4" });

    REQUIRE (hasFailure (checkLoop (verse, "chorus", { verse }, plan), "same as the verse"));
    REQUIRE (checkLoop (loopOf ({ "1", "4", "1", "5" }), "chorus", { verse }, plan).empty ());

    // A colour chord does not make a different progression.
    REQUIRE (hasFailure (checkLoop (loopOf ({ "1sus2", "57", "6m7", "4add9" }), "chorus", { verse }, plan), "same as the verse"));

    // The rule is only for a chorus.
    REQUIRE (checkLoop (verse, "verse", { verse }, plan).empty ());
}

TEST_CASE ("checkLoop needs a tag on a chord that is not diatonic")
{
    const auto plan = planIn ("Major", Difficulty::Intermediate);

    ChordLoop loop = loopOf ({ "1", "b6", "4", "5" });
    REQUIRE (hasFailure (checkLoop (loop, "verse", kNoOtherLoops, plan), "not diatonic and has no tag: b6"));

    loop[1].tags = { "borrowed" };
    REQUIRE (checkLoop (loop, "verse", kNoOtherLoops, plan).empty ());

    loop[1].tags = { "secondary" };
    REQUIRE (checkLoop (loop, "verse", kNoOtherLoops, plan).empty ());

    loop[1].tags = { "pop" };
    REQUIRE_FALSE (checkLoop (loop, "verse", kNoOtherLoops, plan).empty ());
}

TEST_CASE ("checkLoop needs major and minor triads for a Beginner")
{
    const auto beginner = planIn ("Major");

    REQUIRE (hasFailure (checkLoop (loopOf ({ "1", "7dim", "4", "5" }), "verse", kNoOtherLoops, beginner), "major or minor triad: 7dim"));
    REQUIRE (hasFailure (checkLoop (loopOf ({ "1", "7", "4", "5" }), "verse", kNoOtherLoops, beginner), "major or minor triad: 7"));
    REQUIRE (hasFailure (checkLoop (loopOf ({ "1sus2", "6m", "4", "5" }), "verse", kNoOtherLoops, beginner), "major or minor triad: 1sus2"));
    REQUIRE (hasFailure (checkLoop (loopOf ({ "1", "6m", "4", "57" }), "verse", kNoOtherLoops, beginner), "major or minor triad: 57"));

    REQUIRE (checkLoop (loopOf ({ "1sus2", "7", "4", "57" }), "verse", kNoOtherLoops, planIn ("Major", Difficulty::Advanced)).empty ());
}

TEST_CASE ("Each loop from makeLoop passes checkLoop")
{
    const std::vector<std::string> scales { "Major", "Natural Minor", "Dorian", "Mixolydian", "Lydian", "Phrygian", "Locrian", "Harmonic Minor", "Melodic Minor" };

    algo::test::forEachSeed ([&] (std::uint64_t seed)
    {
        const auto plan = planIn (scales[seed % scales.size ()], kDifficulties[seed % kDifficulties.size ()], static_cast<int> (seed % 12));

        Rng        verseRng (seed);
        const auto verse = makeLoop (plan, "verse", kNoOtherLoops, verseRng);
        REQUIRE (checkLoop (verse, "verse", kNoOtherLoops, plan).empty ());

        Rng        chorusRng (seed + 1);
        const auto chorus = makeLoop (plan, "chorus", { verse }, chorusRng);
        REQUIRE (checkLoop (chorus, "chorus", { verse }, plan).empty ());

        Rng        bridgeRng (seed + 2);
        const auto bridge = makeLoop (plan, "bridge", kNoOtherLoops, bridgeRng);
        REQUIRE (checkLoop (bridge, "bridge", kNoOtherLoops, plan).empty ());

        Rng        loopRng (seed + 3);
        const auto loop = makeLoop (plan, "loop", kNoOtherLoops, loopRng);
        REQUIRE (checkLoop (loop, "loop", kNoOtherLoops, plan).empty ());
    });
}

TEST_CASE ("makeLoop uses both the library and the walk")
{
    const auto  plan = planIn ("Major");
    const auto& data = bundledData ();

    int fromLibrary = 0;
    int fromWalk    = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed)
    {
        Rng        rng (seed);
        const auto loop = makeLoop (plan, "verse", kNoOtherLoops, rng);

        // A walk has no chord qualities, so only a library loop can have them.
        if (std::any_of (loop.begin (), loop.end (), [] (const ChordSymbol& chord)
        {
            return chord.quality != kDiatonicQuality;
        }))
            ++fromLibrary;
        else if (!isLibraryLoop (loop, "Major", data))
            ++fromWalk;
    }

    REQUIRE (fromLibrary > 100);
    REQUIRE (fromWalk > 100);
}

TEST_CASE ("A forced failure uses the library after 50 attempts")
{
    const auto plan = planIn ("Major");

    int             calls = 0;
    const LoopCheck check = [&] (const ChordLoop& loop)
    {
        ++calls;
        return calls <= 50 ? std::vector<std::string> { "forced" } : checkLoop (loop, "verse", kNoOtherLoops, plan);
    };

    Rng        rng (1);
    const auto loop = makeLoopWithCheck (plan, "verse", check, rng);

    // The call after the last attempt checks a library progression, and that progression passes.
    REQUIRE (calls == 51);
    REQUIRE (checkLoop (loop, "verse", kNoOtherLoops, plan).empty ());
    REQUIRE (isLibraryLoop (loop, "Major", bundledData ()));
}

TEST_CASE ("The library fallback skips a progression that does not pass")
{
    DataSet data      = loadBundledData ();
    data.progressions = ProgressionLibrary::fromJsonText (R"({ "schemaVersion": 1, "progressions": [
        { "name": "Fails",  "numerals": ["1", "4", "1", "4"], "mode": "Major", "tags": [], "roles": ["chorus"], "difficulty": "Beginner" },
        { "name": "Passes", "numerals": ["1", "6m", "4", "5"], "mode": "Major", "tags": [], "roles": ["chorus"], "difficulty": "Beginner" } ] })",
                                                          data.scales, data.chords);

    const auto plan  = planIn ("Major");
    const auto verse = loopOf ({ "1", "4", "1", "4" });

    LoopRules rules;
    rules.maxAttempts = 0;

    for (std::uint64_t seed = 0; seed < 100; ++seed)
    {
        Rng rng (seed);
        REQUIRE (makeLoop (plan, "chorus", { verse }, rng, data, rules) == loopOf ({ "1", "6m", "4", "5" }));
    }
}

TEST_CASE ("makeLoop throws if no library progression passes after the last attempt")
{
    const auto      plan  = planIn ("Major");
    const LoopCheck check = [] (const ChordLoop&)
    {
        return std::vector<std::string> { "forced" };
    };

    Rng rng (1);
    REQUIRE_THROWS_AS (makeLoopWithCheck (plan, "verse", check, rng), DataError);
}

TEST_CASE ("An 8-bar verse with 1 chord in each bar plays its loop 2 times")
{
    SongPlan plan = planIn ("Major");
    plan.bpm      = 100;
    plan.sections = { { "Verse", "verse", 8 } };

    const auto result = makeChords (plan, [] (Stage stage, int section)
    {
        return rngFor (7, stage, section, 0);
    });

    REQUIRE (result.sections.size () == 1);
    const auto& section = result.sections[0];
    REQUIRE (section.name == "Verse");
    REQUIRE (section.role == "verse");
    REQUIRE (section.bars.size () == 8);

    for (std::size_t bar = 0; bar < section.bars.size (); ++bar)
    {
        REQUIRE (section.bars[bar].chords.size () == 1);
        REQUIRE (section.bars[bar].chords[0] == section.loop[bar % 4]);
        REQUIRE (section.bars[bar].voicings.empty ());
    }
}

TEST_CASE ("Advanced or a BPM below 80 puts 2 chords in each bar")
{
    SongPlan plan = planIn ("Major");
    plan.bpm      = 100;
    REQUIRE (chordsPerBar (plan) == 1);

    plan.bpm = 80;
    REQUIRE (chordsPerBar (plan) == 1);

    plan.bpm = 78;
    REQUIRE (chordsPerBar (plan) == 2);

    plan.bpm        = 100;
    plan.difficulty = Difficulty::Advanced;
    REQUIRE (chordsPerBar (plan) == 2);

    plan.sections     = { { "Bridge", "bridge", 4 } };
    const auto result = makeChords (plan, [] (Stage stage, int section)
    {
        return rngFor (7, stage, section, 0);
    });

    const auto& section = result.sections[0];
    REQUIRE (section.bars.size () == 4);
    for (std::size_t bar = 0; bar < section.bars.size (); ++bar)
    {
        REQUIRE (section.bars[bar].chords.size () == 2);
        REQUIRE (section.bars[bar].chords[0] == section.loop[(bar * 2) % 4]);
        REQUIRE (section.bars[bar].chords[1] == section.loop[(bar * 2 + 1) % 4]);
    }
}

TEST_CASE ("makeChords asks for the stream of the Chords stage for each section")
{
    SongPlan plan = planIn ("Major");
    plan.sections = { { "Verse", "verse", 8 }, { "Chorus", "chorus", 8 }, { "Bridge", "bridge", 4 } };

    std::vector<int> sections;
    makeChords (plan, [&] (Stage stage, int section)
    {
        REQUIRE (stage == Stage::Chords);
        sections.push_back (section);
        return rngFor (1, stage, section, 0);
    });

    REQUIRE (sections == std::vector<int> { 0, 1, 2 });
}

TEST_CASE ("A reroll of section 1 does not change sections 0 and 2")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        const auto difficulty = kDifficulties[seed % kDifficulties.size ()];

        GenerationSettings settings;
        settings.difficulty = difficulty;
        settings.form       = "Verse-Chorus-Bridge";

        Rng        planRng (seed);
        const auto plan = planSong (settings, planRng);

        const auto chordsWith = [&] (int reroll)
        {
            return makeChords (plan, [&] (Stage stage, int section)
            {
                return rngFor (seed, stage, section, section == 1 ? reroll : 0);
            });
        };
        const auto first  = chordsWith (0);
        const auto second = chordsWith (1);

        REQUIRE (first.sections[0].loop == second.sections[0].loop);
        REQUIRE (first.sections[0].bars == second.sections[0].bars);
        REQUIRE (first.sections[2].loop == second.sections[2].loop);
        REQUIRE (first.sections[2].bars == second.sections[2].bars);
    });
}

TEST_CASE ("A reroll of a section can change it")
{
    int changed = 0;
    for (std::uint64_t seed = 0; seed < 100; ++seed)
        if (songFor (seed, Difficulty::Beginner, { 0 }).chords.sections[0].loop != songFor (seed, Difficulty::Beginner, { 1 }).chords.sections[0].loop)
            ++changed;

    REQUIRE (changed > 50);
}

TEST_CASE ("The same seed gives the same chords")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        const auto difficulty = kDifficulties[seed % kDifficulties.size ()];
        const auto a          = songFor (seed, difficulty);
        const auto b          = songFor (seed, difficulty);

        REQUIRE (a.chords.sections.size () == b.chords.sections.size ());
        for (std::size_t i = 0; i < a.chords.sections.size (); ++i)
            REQUIRE (a.chords.sections[i].bars == b.chords.sections[i].bars);
    });
}

TEST_CASE ("In key: each chord of each song is diatonic or has a tag")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        const auto difficulty = kDifficulties[seed % kDifficulties.size ()];
        const auto song       = songFor (seed, difficulty);

        for (const auto& section : song.chords.sections)
        {
            for (const auto& bar : section.bars)
            {
                for (const auto& chord : bar.chords)
                {
                    INFO (numeralText (chord) << " in " << song.plan.key.scale);
                    const bool tagged = chord.tags == std::vector<std::string> { "borrowed" } || chord.tags == std::vector<std::string> { "secondary" };
                    REQUIRE ((isDiatonic (chord, song.plan.key) || tagged));
                }
            }
        }
    });
}

TEST_CASE ("Each section of each song obeys the section rules")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        const auto difficulty = kDifficulties[seed % kDifficulties.size ()];
        const auto song       = songFor (seed, difficulty);

        REQUIRE (song.chords.sections.size () == song.plan.sections.size ());

        std::vector<ChordLoop> verses;
        for (std::size_t i = 0; i < song.chords.sections.size (); ++i)
        {
            const auto& section = song.chords.sections[i];
            REQUIRE (section.name == song.plan.sections[i].name);
            REQUIRE (static_cast<int> (section.bars.size ()) == song.plan.sections[i].bars);
            REQUIRE (section.loop.size () == 4);

            const auto others = section.role == "chorus" ? verses : std::vector<ChordLoop> {};
            REQUIRE (checkLoop (section.loop, section.role, others, song.plan).empty ());

            if (section.role == "verse")
                verses.push_back (section.loop);
            if (section.role == "chorus")
                REQUIRE (rootsOf (section.loop) != rootsOf (verses.at (0)));
        }
    });
}

TEST_CASE ("A scale with no library progressions still gives a song")
{
    for (const char* scale : { "Lydian", "Phrygian", "Locrian", "Harmonic Minor", "Melodic Minor" })
    {
        for (auto difficulty : kDifficulties)
        {
            for (std::uint64_t seed = 0; seed < 100; ++seed)
            {
                GenerationSettings settings;
                settings.scale      = scale;
                settings.difficulty = difficulty;
                settings.form       = "Verse-Chorus-Bridge";

                Rng        planRng (seed);
                const auto plan   = planSong (settings, planRng);
                const auto result = makeChords (plan, [&] (Stage stage, int section)
                {
                    return rngFor (seed, stage, section, 0);
                });

                REQUIRE (plan.key.scale == scale);
                REQUIRE (result.sections.size () == 3);
            }
        }
    }
}

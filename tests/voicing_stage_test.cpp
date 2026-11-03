#include "algo/Chords.h"
#include "algo/Nashville.h"
#include "algo/VoicingCache.h"

#include "support/VoicingHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace algo;
using algo::test::textOf;

namespace
{
    const Key kCMajor { 0, "Major" };

    SectionChords sectionOf (const std::vector<std::string>& numerals, int bars, int perBar = 1)
    {
        SectionChords section;
        section.name = "Verse";
        section.role = "verse";

        for (const auto& numeral : numerals)
            section.loop.push_back (parseNumeral (numeral));

        std::size_t next = 0;
        section.bars.resize (static_cast<std::size_t> (bars));
        for (auto& bar : section.bars)
            for (int i = 0; i < perBar; ++i)
                bar.chords.push_back (section.loop[next++ % section.loop.size ()]);
        return section;
    }

    std::string textOf (const SectionChords& section)
    {
        std::string text;
        for (const auto& bar : section.bars)
            for (const auto& voicing : bar.voicings)
                text += algo::test::textOf (voicing) + " ";
        return text;
    }
}

TEST_CASE ("The concrete chord comes from the symbol and the key")
{
    const auto am = concreteChord (parseNumeral ("6"), kCMajor);
    REQUIRE (am.root == 9);
    REQUIRE (am.definition.name == "Minor");
    REQUIRE (am.name == "Am");
    REQUIRE_FALSE (am.bass.has_value ());

    const auto gOverB = concreteChord (parseNumeral ("5/7"), kCMajor);
    REQUIRE (gOverB.root == 7);
    REQUIRE (gOverB.bass == 11);
    REQUIRE (gOverB.name == "G/B");
}

TEST_CASE ("Each chord in each bar gets a voicing with fingers")
{
    VoicingCache cache;
    const auto   voiced = voiceSection (sectionOf ({ "1", "4", "5", "1" }, 8, 2), kCMajor, Fretboard {}, Difficulty::Beginner, 0, cache);

    REQUIRE (voiced.bars.size () == 8);
    for (const auto& bar : voiced.bars)
    {
        REQUIRE (bar.voicings.size () == bar.chords.size ());
        for (const auto& voicing : bar.voicings)
        {
            const auto fingered = assignFingers (voicing);
            REQUIRE (fingered.has_value ());
            REQUIRE (fingered->fingers == voicing.fingers);
        }
    }
}

TEST_CASE ("The same chord position has the same voicing in each pass of the loop")
{
    VoicingCache cache;
    const auto   voiced = voiceSection (sectionOf ({ "1", "6", "4", "5" }, 8), kCMajor, Fretboard {}, Difficulty::Intermediate, 2, cache);

    for (std::size_t bar = 4; bar < voiced.bars.size (); ++bar)
        REQUIRE (voiced.bars[bar].voicings == voiced.bars[bar - 4].voicings);
}

TEST_CASE ("A reroll count of 0 to 4 gives 5 different sequences")
{
    const auto section = sectionOf ({ "1", "6", "4", "5" }, 8);

    for (const auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
    {
        VoicingCache cache;

        std::set<std::string> texts;
        for (int reroll = 0; reroll <= 4; ++reroll)
            texts.insert (textOf (voiceSection (section, kCMajor, Fretboard {}, difficulty, reroll, cache)));

        REQUIRE (texts.size () == 5);
    }
}

TEST_CASE ("A reroll count above 4 starts again and a negative count is not valid")
{
    const auto   section = sectionOf ({ "1", "6", "4", "5" }, 4);
    VoicingCache cache;

    REQUIRE (textOf (voiceSection (section, kCMajor, Fretboard {}, Difficulty::Beginner, 5, cache)) == textOf (voiceSection (section, kCMajor, Fretboard {}, Difficulty::Beginner, 0, cache)));
    REQUIRE_THROWS_AS (voiceSection (section, kCMajor, Fretboard {}, Difficulty::Beginner, -1, cache), std::invalid_argument);
}

TEST_CASE ("A second call with the same chords does not run the search again")
{
    const auto   section = sectionOf ({ "1", "6", "4", "5" }, 8);
    VoicingCache cache;

    voiceSection (section, kCMajor, Fretboard {}, Difficulty::Beginner, 0, cache);
    REQUIRE (cache.searchCount () == 4);

    voiceSection (section, kCMajor, Fretboard {}, Difficulty::Beginner, 3, cache);
    REQUIRE (cache.searchCount () == 4);

    SECTION ("A different difficulty runs the search")
    {
        voiceSection (section, kCMajor, Fretboard {}, Difficulty::Advanced, 0, cache);
        REQUIRE (cache.searchCount () == 8);
    }

    SECTION ("A different capo runs the search")
    {
        voiceSection (section, kCMajor, Fretboard (Tuning {}, 2), Difficulty::Beginner, 0, cache);
        REQUIRE (cache.searchCount () == 8);
    }

    SECTION ("A different tuning runs the search")
    {
        Tuning dropD;
        dropD.strings[0] = 38;
        voiceSection (section, kCMajor, Fretboard (dropD), Difficulty::Beginner, 0, cache);
        REQUIRE (cache.searchCount () == 8);
    }
}

TEST_CASE ("The stage voices all sections with their own reroll counts")
{
    ProgressionSet progressions;
    progressions.sections.push_back (sectionOf ({ "1", "6", "4", "5" }, 4));
    progressions.sections.push_back (sectionOf ({ "4", "5", "6", "1" }, 4));

    VoicingCache cache;

    const auto first  = voiceChords (progressions, kCMajor, Fretboard {}, Difficulty::Beginner, {}, cache);
    const auto second = voiceChords (progressions, kCMajor, Fretboard {}, Difficulty::Beginner, { 0, 1 }, cache);

    REQUIRE (first.sections.size () == 2);
    REQUIRE (textOf (first.sections[0]) == textOf (second.sections[0]));
    REQUIRE (textOf (first.sections[1]) != textOf (second.sections[1]));
}

TEST_CASE ("The stage does not change the chords of the section")
{
    const auto   section = sectionOf ({ "1", "6", "4", "5" }, 4);
    VoicingCache cache;
    const auto   voiced = voiceSection (section, kCMajor, Fretboard {}, Difficulty::Beginner, 0, cache);

    REQUIRE (voiced.name == section.name);
    REQUIRE (voiced.loop == section.loop);
    for (std::size_t bar = 0; bar < section.bars.size (); ++bar)
        REQUIRE (voiced.bars[bar].chords == section.bars[bar].chords);
}

TEST_CASE ("A section with no bars is not an error")
{
    VoicingCache cache;
    REQUIRE (voiceSection (SectionChords {}, kCMajor, Fretboard {}, Difficulty::Beginner, 0, cache).bars.empty ());
}

TEST_CASE ("A Beginner chord uses the shapes if a shape passes the filter")
{
    VoicingCache cache;

    const auto& bm = cache.candidates (algo::test::chordOf (11, "Minor"), Difficulty::Beginner, Fretboard {});
    REQUIRE (algo::test::hasVoicing (bm, "x2443x"));
    REQUIRE_FALSE (algo::test::hasVoicing (bm, "x20402"));

    const auto& advanced = cache.candidates (algo::test::chordOf (11, "Minor"), Difficulty::Advanced, Fretboard {});
    REQUIRE (algo::test::hasVoicing (advanced, "x20402"));

    // No shape has this chord at a low position with 4 strings, so the search must help.
    const auto& dim = cache.candidates (algo::test::chordOf (0, "Dim"), Difficulty::Beginner, Fretboard {});
    REQUIRE_FALSE (dim.empty ());
}

TEST_CASE ("Each chord type has a voicing on each root above Beginner")
{
    const Fretboard fretboard;
    VoicingCache    cache;

    for (const auto difficulty : { Difficulty::Intermediate, Difficulty::Advanced })
        for (PitchClass root = 0; root < 12; ++root)
            for (const auto& definition : bundledData ().chords.all ())
            {
                CAPTURE (root, definition.name);
                REQUIRE_FALSE (cache.candidates (algo::test::chordOf (root, definition.name), difficulty, fretboard).empty ());
            }
}

TEST_CASE ("Each triad has a voicing on each root and capo at Beginner")
{
    VoicingCache cache;

    for (int capo = 0; capo <= Fretboard::kMaxCapo; ++capo)
    {
        const Fretboard fretboard (Tuning {}, capo);

        for (PitchClass root = 0; root < 12; ++root)
            for (const char* type : { "Major", "Minor" })
            {
                CAPTURE (capo, root, type);
                REQUIRE_FALSE (cache.candidates (algo::test::chordOf (root, type), Difficulty::Beginner, fretboard).empty ());
            }
    }
}

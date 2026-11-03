#include "algo/Engine.h"
#include "algo/Strum.h"
#include "algo/Tab.h"

#include "support/Golden.h"
#include "support/StarterHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace algo;
using Catch::Matchers::ContainsSubstring;

namespace
{
    Bar barOf (const std::array<int, kNumStrings>& frets)
    {
        Bar bar;
        bar.chords.push_back (ChordSymbol {});
        Voicing voicing;
        voicing.frets = frets;
        bar.voicings.push_back (voicing);
        return bar;
    }

    StrumPattern strumOf (const std::string& text)
    {
        StrumPattern pattern;
        pattern.steps = static_cast<int> (text.size ());
        pattern.slots.clear ();
        for (char c : text)
            pattern.slots.push_back (*parseStroke (c));
        pattern.accent.assign (text.size (), 0.6);
        return pattern;
    }
}

TEST_CASE ("The example bar: a G chord with the strum D - D U - U D U")
{
    const std::string expected = "e|-3---3-3---3-3-3-|\n"
                                 "B|-0---0-0---0-0-0-|\n"
                                 "G|-0---0-0---0-0-0-|\n"
                                 "D|-0---0-0---0-0-0-|\n"
                                 "A|-2---2-2---2-2-2-|\n"
                                 "E|-3---3-3---3-3-3-|\n"
                                 "\n";
    REQUIRE (tabBar (barOf ({ 3, 2, 0, 0, 0, 3 }), strumOf ("D-DU-UDU")) == expected);
}

TEST_CASE ("A Mute is x, a muted string is - and a 2-digit fret makes each column 2 characters wide")
{
    const std::string expected = "e|--8--x----|\n"
                                 "B|--8--x----|\n"
                                 "G|--9--x----|\n"
                                 "D|-10--x----|\n"
                                 "A|-10--x----|\n"
                                 "E|--8--x----|\n"
                                 "\n";
    REQUIRE (tabBar (barOf ({ 8, 10, 10, 9, 8, 8 }), strumOf ("Dx-")) == expected);

    const std::string partial = "e|-0-|\n"
                                "B|-1-|\n"
                                "G|-0-|\n"
                                "D|-2-|\n"
                                "A|---|\n"
                                "E|---|\n"
                                "\n";
    auto              strum   = strumOf ("D");
    strum.strings             = 4;
    REQUIRE (tabBar (barOf ({ kMutedString, 3, 2, 0, 1, 0 }), strum) == partial);
}

TEST_CASE ("A chord diagram lists the frets from the low E string")
{
    Voicing f;
    f.frets = { 1, 3, 3, 2, 1, 1 };
    REQUIRE (chordDiagram (f) == "133211");

    Voicing high;
    high.frets = { kMutedString, 12, 14, 14, 13, 12 };
    REQUIRE (chordDiagram (high) == "x[12][14][14][13][12]");
}

TEST_CASE ("The tab has a header, a chord chart and a strum line for each section")
{
    GenerationSettings settings;
    settings.masterSeed = 81723;
    const auto starter  = generate (settings);
    const auto tab      = writeTab (starter);

    for (const auto& section : starter.sections)
    {
        REQUIRE_THAT (tab, ContainsSubstring ("[" + section.name + "] " + std::to_string (starter.bpm) + " BPM | Key: "));
        REQUIRE_THAT (tab, ContainsSubstring ("Strum: " + strumText (section.strum)));
    }
    REQUIRE_THAT (tab, ContainsSubstring ("Chords: | G (1) |"));
    REQUIRE_THAT (tab, ContainsSubstring ("Chord shapes (low E to high E):"));
}

TEST_CASE ("A song starter with a hook has single-note hook tab")
{
    auto       settings = algo::test::settingsFor (5, Difficulty::Intermediate, true);
    const auto starter  = generate (settings);
    REQUIRE_FALSE (starter.hook.empty ());
    REQUIRE_THAT (writeTab (starter), ContainsSubstring ("Hook (bars 1-"));
}

TEST_CASE ("5 golden tab files match the output byte for byte")
{
    for (const auto& [name, settings] : algo::test::goldenSettings ())
    {
        CAPTURE (name);
        algo::test::requireGolden ("tab/" + name + ".txt", writeTab (generate (settings)));
    }
}

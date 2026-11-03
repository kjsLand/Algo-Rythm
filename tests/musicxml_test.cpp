#include "algo/Engine.h"
#include "algo/MusicXml.h"

#include "support/Golden.h"
#include "support/StarterHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <regex>

using namespace algo;
using Catch::Matchers::ContainsSubstring;

namespace
{
    std::size_t countOf (const std::string& text, const std::string& part)
    {
        std::size_t count = 0;
        for (auto pos = text.find (part); pos != std::string::npos; pos = text.find (part, pos + 1))
            ++count;
        return count;
    }
}

TEST_CASE ("The score has a notation staff and a 6-line tab staff in the tuning of the song starter")
{
    const auto starter = generate (GenerationSettings {});
    const auto xml     = writeMusicXml (starter);

    REQUIRE_THAT (xml, ContainsSubstring ("<score-partwise version=\"4.0\">"));
    REQUIRE_THAT (xml, ContainsSubstring ("<staves>2</staves>"));
    REQUIRE_THAT (xml, ContainsSubstring ("<sign>TAB</sign>"));
    REQUIRE_THAT (xml, ContainsSubstring ("<staff-lines>6</staff-lines>"));
    REQUIRE_THAT (xml, ContainsSubstring ("<staff-tuning line=\"1\"><tuning-step>E</tuning-step><tuning-octave>2</tuning-octave></staff-tuning>"));
    REQUIRE_THAT (xml, ContainsSubstring ("<harmony"));
    REQUIRE_THAT (xml, ContainsSubstring ("<technical><string>"));
    REQUIRE (countOf (xml, "<measure ") == 16);
    REQUIRE (countOf (xml, "<score-part ") == 1);
}

TEST_CASE ("A hook is a second part with the same number of measures")
{
    const auto starter = generate (algo::test::settingsFor (2, Difficulty::Beginner, true));
    const auto xml     = writeMusicXml (starter);
    REQUIRE (countOf (xml, "<score-part ") == 2);
    REQUIRE_THAT (xml, ContainsSubstring ("<part id=\"P2\">"));

    int bars = 0;
    for (const auto& section : starter.sections)
        bars += static_cast<int> (section.bars.size ());
    REQUIRE (countOf (xml, "<measure ") == static_cast<std::size_t> (2 * bars));
}

TEST_CASE ("A DADGAD song starter writes the DADGAD tuning")
{
    GenerationSettings settings;
    settings.tuning = "DADGAD";
    const auto xml  = writeMusicXml (generate (settings));
    REQUIRE_THAT (xml, ContainsSubstring ("<staff-tuning line=\"1\"><tuning-step>D</tuning-step><tuning-octave>2</tuning-octave></staff-tuning>"));
    REQUIRE_THAT (xml, ContainsSubstring ("<staff-tuning line=\"5\"><tuning-step>A</tuning-step><tuning-octave>3</tuning-octave></staff-tuning>"));
}

TEST_CASE ("The duration of each guitar measure adds up to the time signature")
{
    for (const auto& [name, settings] : algo::test::goldenSettings ())
    {
        CAPTURE (name);
        const auto xml = writeMusicXml (generate (settings));
        algo::test::requireGolden ("musicxml/" + name + ".musicxml", xml);

        // Each measure has 2 voices, each with 16 divisions (4/4 at 4 divisions for each quarter note).
        const std::regex measure ("<measure number=\"[0-9]+\">([\\s\\S]*?)</measure>");
        const std::regex duration ("<note>(?:(?!<chord/>)[\\s\\S])*?<duration>([0-9]+)</duration>");
        for (auto it = std::sregex_iterator (xml.begin (), xml.end (), measure); it != std::sregex_iterator (); ++it)
        {
            const std::string body  = (*it)[1];
            int               total = 0;
            for (auto n = std::sregex_iterator (body.begin (), body.end (), duration); n != std::sregex_iterator (); ++n)
                total += std::stoi ((*n)[1]);
            REQUIRE (total == 32);
        }
    }
}

#include "AbTest.h"
#include "Library.h"
#include "Theme.h"

#include "algo/Engine.h"
#include "algo/Files.h"
#include "algo/Ratings.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE ("Each text colour has a contrast ratio of 4.5:1 or more (REQ-11-6)")
{
    const std::vector<std::pair<juce::Colour, juce::Colour>> pairs {
        { Theme::text, Theme::background },
        { Theme::textDim, Theme::background },
        { Theme::text, Theme::panel },
        { Theme::textDim, Theme::panel },
        { Theme::text, Theme::panelRaised },
        { Theme::textDim, Theme::panelRaised },
        { Theme::chordText, Theme::accent },
        { Theme::chordText, Theme::root },
        { Theme::scaleText, Theme::scale },
        { Theme::warning, Theme::background },
        { Theme::warning, Theme::panel },
        { Theme::accent, Theme::panel },
    };
    for (const auto& [foreground, background] : pairs)
    {
        CAPTURE (foreground.toDisplayString (false), background.toDisplayString (false));
        REQUIRE (Theme::contrast (foreground, background) >= 4.5);
    }
}

TEST_CASE ("Over 100 A/B clicks, each table type occurs 40 to 60 times")
{
    for (const juce::int64 seed : { 1, 2, 3, 42, 2026 })
    {
        juce::Random random (seed);
        int          learned = 0;
        for (int click = 0; click < 100; ++click)
            learned += AbTest::chooseTables (random) == "learned" ? 1 : 0;
        CAPTURE (seed);
        REQUIRE (learned >= 40);
        REQUIRE (learned <= 60);
    }
}

TEST_CASE ("20 saved song starters reload to the same result (REQ-14-3)")
{
    const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("algo-library-test");
    root.deleteRecursively ();
    Library library (root);

    std::vector<algo::SongStarter> saved;
    for (std::uint64_t seed = 1; seed <= 20; ++seed)
    {
        algo::GenerationSettings settings;
        settings.masterSeed = seed;
        settings.hook       = seed % 2 == 0;
        saved.push_back (algo::generate (settings));
        library.save (saved.back ());
    }

    const auto entries = library.list ();
    REQUIRE (entries.size () == 20);
    for (const auto& starter : saved)
    {
        const auto file = library.folder ().getChildFile (juce::String (algo::defaultFileName (starter)) + ".json");
        REQUIRE (library.load (file) == starter);
    }
    root.deleteRecursively ();
}

TEST_CASE ("Each rating adds one line to ratings.csv, after the header")
{
    const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("algo-ratings-test");
    root.deleteRecursively ();
    Library library (root);

    auto starter   = algo::generate (algo::GenerationSettings {});
    starter.rating = 4;
    library.appendRating (starter, "");
    starter.rating = 2;
    library.appendRating (starter, "rules");

    juce::StringArray lines;
    lines.addLines (library.ratingsFile ().loadFileAsString ().trim ());
    REQUIRE (lines.size () == 3);
    REQUIRE (lines[0] == algo::kRatingsHeader);
    REQUIRE (lines[1].contains (",4,"));
    REQUIRE (lines[2].endsWith (",rules"));

    starter.rating = 0;
    REQUIRE_THROWS (library.appendRating (starter, ""));
    root.deleteRecursively ();
}

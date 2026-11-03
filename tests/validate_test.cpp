#include "algo/Engine.h"
#include "algo/Nashville.h"
#include "algo/Strum.h"
#include "algo/Validate.h"
#include "algo/Voicing.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

using namespace algo;

namespace
{
    bool hasRule (const std::vector<Failure>& failures, const std::string& rule)
    {
        return std::any_of (failures.begin (), failures.end (), [&rule] (const Failure& f)
        {
            return f.rule == rule;
        });
    }
}

TEST_CASE ("A generated song starter passes and a changed one fails")
{
    auto starter = generate (GenerationSettings {});
    REQUIRE (validateStarter (starter).empty ());

    // A fret that is not on the neck is a second note on the string.
    auto broken                                     = starter;
    broken.sections[0].bars[0].voicings[0].frets[0] = 30;
    const auto failures                             = validateStarter (broken);
    REQUIRE (hasRule (failures, kRuleOneNotePerString));
    REQUIRE (failures[0].section == starter.sections[0].name);
    REQUIRE (failures[0].bar == 0);
    REQUIRE (failureText (failures[0]).rfind (starter.sections[0].name + " bar 1: one-note-per-string", 0) == 0);
}

TEST_CASE ("The strum rules find each broken requirement")
{
    StrumPattern pattern;
    pattern.steps   = 8;
    pattern.slots   = { Stroke::Rest, Stroke::Down, Stroke::Up, Stroke::Rest, Stroke::Rest, Stroke::Mute, Stroke::Rest, Stroke::Rest };
    pattern.accent  = std::vector<double> (8, 2.0);
    pattern.strings = 4;

    const auto rules = strumViolations (pattern, Difficulty::Beginner, 100);
    for (const auto* rule : { kRuleStrumDownbeat, kRuleStrumDirection, kRuleStrumBeat3, kRuleStrumPercussion, kRuleStrumAccent, kRuleStrumStrings })
        REQUIRE (std::find (rules.begin (), rules.end (), rule) != rules.end ());

    pattern.steps   = 16;
    const auto grid = strumViolations (pattern, Difficulty::Beginner, 100);
    REQUIRE (std::find (grid.begin (), grid.end (), kRuleStrumGrid) != grid.end ());
}

TEST_CASE ("A chord that is not in the key needs a tag, and a rating must be 1 to 5")
{
    auto starter                          = generate (GenerationSettings {});
    starter.sections[0].bars[0].chords[0] = parseNumeral ("b7"); // No "borrowed" tag
    starter.rating                        = 9;

    const auto failures = validateStarter (starter);
    REQUIRE (hasRule (failures, kRuleChordTag));
    REQUIRE (hasRule (failures, kRuleRating));
}

TEST_CASE ("A hook note out of the scale or too long fails")
{
    GenerationSettings settings;
    settings.hook = true;
    auto starter  = generate (settings);
    REQUIRE (hookViolations (starter).empty ());

    auto broken = starter;
    broken.hook[1].pitch += 1;
    broken.hook[2].length = 2000;
    const auto failures   = hookViolations (broken);
    REQUIRE (hasRule (failures, kRuleHookLength));
    REQUIRE ((hasRule (failures, kRuleHookScale) || hasRule (failures, kRuleHookChordTone)));
}

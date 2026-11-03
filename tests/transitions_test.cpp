#include "algo/Data.h"
#include "algo/DataError.h"
#include "algo/Transitions.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <array>
#include <cmath>
#include <set>
#include <stdexcept>
#include <string>

using namespace algo;
using Catch::Matchers::ContainsSubstring;

namespace
{
    constexpr std::array<TransitionMode, 2> kModes { TransitionMode::Major, TransitionMode::Minor };

    constexpr NumeralMask kAllNumerals { true, true, true, true, true, true, true };

    // A table where each numeral goes to the numeral after it.
    std::string tableText (const std::string& firstRow, const std::string& source = R"("source": "test",)")
    {
        const std::string rows = R"({ "1": )" + firstRow + R"(, "2": { "3": 1.0 }, "3": { "4": 1.0 }, "4": { "5": 1.0 }, "5": { "6": 1.0 }, "6": { "7": 1.0 }, "7": { "1": 1.0 } })";
        return R"({ "schemaVersion": 1, )" + source + R"( "tables": [ { "name": "major", "rows": )" + rows + R"( }, { "name": "minor", "rows": )" + rows + " } ] }";
    }

    void requireTableError (const std::string& text, const std::string& entry, const std::string& problem)
    {
        try
        {
            TransitionTable::fromJsonText (text);
            FAIL ("No error");
        }
        catch (const DataError& e)
        {
            REQUIRE (e.file == "transitions.json");
            REQUIRE (e.entry == entry);
            REQUIRE_THAT (e.message, ContainsSubstring (problem));
        }
    }
}

TEST_CASE ("Each diatonic numeral has a row in each bundled table")
{
    const auto& table = bundledData ().transitions;

    for (auto mode : kModes)
    {
        for (int from = 1; from <= kDiatonicNumerals; ++from)
        {
            double sum = 0.0;
            for (int to = 1; to <= kDiatonicNumerals; ++to)
                sum += table.weight (mode, from, to);

            INFO ("Numeral " << from);
            REQUIRE (std::abs (sum - 1.0) <= 0.001);
        }
    }
}

TEST_CASE ("The bundled table gives 5 after 1 the weight from the Hooktheory data")
{
    REQUIRE (bundledData ().transitions.weight (TransitionMode::Major, 1, 5) == Catch::Approx (0.31));
}

TEST_CASE ("The bundled table has its source")
{
    REQUIRE (bundledData ().transitions.source () == "hand-authored v1");
}

TEST_CASE ("A row that does not add up to 1.0 causes an error")
{
    requireTableError (tableText (R"({ "2": 0.9 })"), "major", "adds up to 0.9");
    requireTableError (tableText (R"({ "2": 1.1 })"), "major", "adds up to 1.1");

    REQUIRE_NOTHROW (TransitionTable::fromJsonText (tableText (R"({ "2": 1.0005 })")));
    REQUIRE_NOTHROW (TransitionTable::fromJsonText (tableText (R"({ "2": 0.5, "3": 0.5 })")));
}

TEST_CASE ("A transition table that is not complete causes an error")
{
    const std::string row = R"({ "1": { "2": 1.0 }, "2": { "1": 1.0 }, "3": { "1": 1.0 }, "4": { "1": 1.0 }, "5": { "1": 1.0 }, "6": { "1": 1.0 }, "7": { "1": 1.0 } })";

    requireTableError (R"({ "schemaVersion": 1, "source": "test", "tables": [ { "name": "major", "rows": )" + row + " } ] }", "", "'minor' is missing");
    requireTableError (R"({ "schemaVersion": 1, "source": "test", "tables": [ { "name": "minor", "rows": )" + row + " } ] }", "", "'major' is missing");
    requireTableError (R"({ "schemaVersion": 1, "source": "test", "tables": [ { "name": "major", "rows": { "1": { "2": 1.0 } } } ] })", "major", "numeral '2' has no row");
    requireTableError (R"({ "schemaVersion": 1, "source": "test", "tables": [ { "name": "lydian", "rows": {} } ] })", "lydian", "must be 'major' or 'minor'");
    requireTableError (R"({ "schemaVersion": 1, "source": "test", "tables": [ { "name": "major", "rows": 5 } ] })", "major", "'rows' must be an object");
}

TEST_CASE ("A value that is not valid in a transition table causes an error")
{
    requireTableError (tableText (R"({ "8": 1.0 })"), "major", "'8', which is not a diatonic numeral");
    requireTableError (tableText (R"({ "b7": 1.0 })"), "major", "'b7', which is not a diatonic numeral");
    requireTableError (tableText (R"({ "2": "1.0" })"), "major", "weights of 0 or more");
    requireTableError (tableText (R"({ "2": 1.5, "3": -0.5 })"), "major", "weights of 0 or more");
    requireTableError (tableText ("5"), "major", "row '1' must be an object");
}

TEST_CASE ("A transition table with no source causes an error")
{
    requireTableError (tableText (R"({ "2": 1.0 })", ""), "", "'source' is missing");
    requireTableError (tableText (R"({ "2": 1.0 })", R"("source": 5,)"), "", "'source' must be a string");
}

TEST_CASE ("next uses the weights of its row")
{
    const auto table = TransitionTable::fromJsonText (tableText (R"({ "2": 0.25, "5": 0.75 })"));
    Rng        rng (1);

    int fives = 0;
    for (int i = 0; i < 10000; ++i)
    {
        const auto next = table.next (TransitionMode::Major, 1, kAllNumerals, rng);
        REQUIRE ((next == 2 || next == 5));
        fives += next == 5 ? 1 : 0;
    }

    REQUIRE (fives >= 7250);
    REQUIRE (fives <= 7750);
}

TEST_CASE ("next never returns the same numeral")
{
    const auto table = TransitionTable::fromJsonText (tableText (R"({ "1": 0.5, "2": 0.5 })"));
    Rng        rng (1);

    for (int i = 0; i < 1000; ++i)
        REQUIRE (table.next (TransitionMode::Major, 1, kAllNumerals, rng) == 2);
}

TEST_CASE ("next returns only allowed numerals")
{
    const auto& table = bundledData ().transitions;
    Rng         rng (1);

    NumeralMask mask = kAllNumerals;
    mask[6]          = false;

    std::set<int> seen;
    for (int i = 0; i < 1000; ++i)
        seen.insert (*table.next (TransitionMode::Major, 1, mask, rng));

    REQUIRE (seen == std::set<int> { 2, 3, 4, 5, 6 });
}

TEST_CASE ("next returns no value if no numeral has a weight")
{
    const auto table = TransitionTable::fromJsonText (tableText (R"({ "2": 1.0 })"));
    Rng        rng (1);

    NumeralMask mask = kAllNumerals;
    mask[1]          = false;

    REQUIRE_FALSE (table.next (TransitionMode::Major, 1, mask, rng).has_value ());
}

TEST_CASE ("The major and minor tables can differ")
{
    const auto& table = bundledData ().transitions;

    REQUIRE (table.weight (TransitionMode::Major, 1, 7) != table.weight (TransitionMode::Minor, 1, 7));
}

TEST_CASE ("weight throws for a numeral that is not valid")
{
    const auto& table = bundledData ().transitions;

    REQUIRE_THROWS_AS (table.weight (TransitionMode::Major, 0, 1), std::out_of_range);
    REQUIRE_THROWS_AS (table.weight (TransitionMode::Major, 1, 8), std::out_of_range);
}

#include "algo/Rng.h"

#include "support/PropertyTest.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using algo::Rng;

// The stored values in this file come from a separate Python program that follows the published
// definition of the engine. They do not come from this code.

TEST_CASE ("uniformInt gives the stored values for seed 42")
{
    Rng rng (42);

    std::vector<int> values;
    for (int i = 0; i < 10; ++i)
        values.push_back (rng.uniformInt (0, 99));

    REQUIRE (values == std::vector<int> { 6, 24, 50, 62, 81, 28, 36, 44, 50, 57 });
}

TEST_CASE ("uniformInt gives each value about the same number of times")
{
    Rng                 rng (42);
    std::array<int, 10> counts {};

    for (int i = 0; i < 100000; ++i)
        ++counts[static_cast<std::size_t> (rng.uniformInt (0, 9))];

    for (std::size_t value = 0; value < counts.size (); ++value)
    {
        CAPTURE (value, counts[value]);
        REQUIRE (counts[value] >= 9500);
        REQUIRE (counts[value] <= 10500);
    }
}

TEST_CASE ("uniformInt stays in its range")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        Rng rng (seed);

        for (int i = 0; i < 20; ++i)
        {
            const int value = rng.uniformInt (-3, 3);
            REQUIRE (value >= -3);
            REQUIRE (value <= 3);
        }
    });
}

TEST_CASE ("uniformInt accepts a range with one value and the full int range")
{
    Rng rng (1);

    REQUIRE (rng.uniformInt (5, 5) == 5);
    REQUIRE (rng.uniformInt (-5, -5) == -5);

    constexpr int lowest  = std::numeric_limits<int>::lowest ();
    constexpr int highest = std::numeric_limits<int>::max ();

    bool sawNegative = false;
    bool sawPositive = false;

    for (int i = 0; i < 100; ++i)
    {
        const int value = rng.uniformInt (lowest, highest);
        sawNegative     = sawNegative || value < 0;
        sawPositive     = sawPositive || value > 0;
    }

    REQUIRE (sawNegative);
    REQUIRE (sawPositive);
}

TEST_CASE ("uniformInt throws if lo is more than hi")
{
    Rng rng (1);

    REQUIRE_THROWS_AS (rng.uniformInt (1, 0), std::invalid_argument);
}

TEST_CASE ("uniformReal uses the top 53 bits of one engine output")
{
    Rng rng (42);

    for (std::uint64_t expected : { 6801836353641660ULL, 5755883094484128ULL, 6774721691634639ULL })
        REQUIRE (static_cast<std::uint64_t> (rng.uniformReal () * 0x1p53) == expected);
}

TEST_CASE ("uniformReal stays in [0, 1)")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        Rng rng (seed);

        for (int i = 0; i < 20; ++i)
        {
            const double value = rng.uniformReal ();
            REQUIRE (value >= 0.0);
            REQUIRE (value < 1.0);
        }
    });
}

TEST_CASE ("chance uses its probability")
{
    Rng rng (42);

    int hits = 0;
    for (int i = 0; i < 100000; ++i)
        if (rng.chance (0.25))
            ++hits;

    CAPTURE (hits);
    REQUIRE (hits >= 24000);
    REQUIRE (hits <= 26000);
}

TEST_CASE ("chance with p 0 is never true and with p 1 is always true")
{
    Rng rng (42);

    for (int i = 0; i < 10000; ++i)
    {
        REQUIRE_FALSE (rng.chance (0.0));
        REQUIRE (rng.chance (1.0));
    }
}

TEST_CASE ("chance uses one engine output for all values of p")
{
    Rng a (42);
    Rng b (42);

    a.chance (0.0);
    b.chance (1.0);

    REQUIRE (a.uniformInt (0, 1000000) == b.uniformInt (0, 1000000));
}

TEST_CASE ("weightedIndex follows the weights")
{
    Rng                rng (42);
    std::array<int, 4> counts {};

    for (int i = 0; i < 100000; ++i)
        ++counts[rng.weightedIndex ({ 1.0, 0.0, 3.0, 0.0 })];

    CAPTURE (counts[0], counts[2]);
    REQUIRE (counts[0] >= 24000);
    REQUIRE (counts[0] <= 26000);
    REQUIRE (counts[1] == 0);
    REQUIRE (counts[2] >= 74000);
    REQUIRE (counts[2] <= 76000);
    REQUIRE (counts[3] == 0);
}

TEST_CASE ("weightedIndex throws if it cannot choose an index")
{
    Rng        rng (1);
    const auto nan = std::numeric_limits<double>::quiet_NaN ();
    const auto inf = std::numeric_limits<double>::infinity ();
    const auto big = std::numeric_limits<double>::max ();

    REQUIRE_THROWS_AS (rng.weightedIndex ({}), std::invalid_argument);
    REQUIRE_THROWS_AS (rng.weightedIndex ({ 0.0, 0.0 }), std::invalid_argument);
    REQUIRE_THROWS_AS (rng.weightedIndex ({ 1.0, -0.5 }), std::invalid_argument);
    REQUIRE_THROWS_AS (rng.weightedIndex ({ 1.0, nan }), std::invalid_argument);
    REQUIRE_THROWS_AS (rng.weightedIndex ({ 1.0, inf }), std::invalid_argument);
    REQUIRE_THROWS_AS (rng.weightedIndex ({ big, big }), std::invalid_argument);
}

TEST_CASE ("shuffle gives the stored order for seed 7")
{
    Rng              rng (7);
    std::vector<int> items (10);
    std::iota (items.begin (), items.end (), 0);

    rng.shuffle (items);

    REQUIRE (items == std::vector<int> { 0, 7, 4, 9, 3, 1, 2, 8, 6, 5 });
}

TEST_CASE ("shuffle keeps all items")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        Rng              rng (seed);
        std::vector<int> items (20);
        std::iota (items.begin (), items.end (), 0);
        const auto original = items;

        rng.shuffle (items);
        std::sort (items.begin (), items.end ());

        REQUIRE (items == original);
    });
}

TEST_CASE ("shuffle accepts a short list and uses no value")
{
    Rng a (42);
    Rng b (42);

    std::vector<std::string> empty;
    std::vector<std::string> one { "x" };
    a.shuffle (empty);
    a.shuffle (one);

    REQUIRE (empty.empty ());
    REQUIRE (one == std::vector<std::string> { "x" });
    REQUIRE (a.uniformInt (0, 1000000) == b.uniformInt (0, 1000000));
}

TEST_CASE ("the same seed gives the same values and another seed gives other values")
{
    Rng a (42);
    Rng b (42);
    Rng c (43);

    bool anyDifferent = false;

    for (int i = 0; i < 100; ++i)
    {
        const int value = a.uniformInt (0, 1000000);
        REQUIRE (value == b.uniformInt (0, 1000000));
        anyDifferent = anyDifferent || value != c.uniformInt (0, 1000000);
    }

    REQUIRE (anyDifferent);
}

TEST_CASE ("an Rng can move and keeps its position, but cannot be copied")
{
    STATIC_REQUIRE_FALSE (std::is_copy_constructible<Rng>::value);
    STATIC_REQUIRE_FALSE (std::is_copy_assignable<Rng>::value);

    Rng a (42);
    Rng b (42);

    REQUIRE (a.uniformInt (0, 99) == b.uniformInt (0, 99));

    Rng moved (std::move (a));
    REQUIRE (moved.uniformInt (0, 99) == b.uniformInt (0, 99));

    Rng assigned (1);
    assigned = std::move (moved);
    REQUIRE (assigned.uniformInt (0, 99) == b.uniformInt (0, 99));
}

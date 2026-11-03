#include "algo/Euclid.h"

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>
#include <vector>

using algo::bjorklund;
using algo::hitCount;

namespace
{
    std::string text (const std::vector<int>& pattern)
    {
        std::string out;
        for (int slot : pattern)
            out += slot ? 'X' : '.';
        return out;
    }
}

TEST_CASE ("bjorklund gives the classic Euclidean rhythms")
{
    REQUIRE (text (bjorklund (3, 8)) == "X..X..X.");
    REQUIRE (text (bjorklund (5, 8)) == "X.XX.XX.");
    REQUIRE (text (bjorklund (0, 4)) == "....");
    REQUIRE (text (bjorklund (4, 4)) == "XXXX");
}

TEST_CASE ("bjorklund keeps the number of hits and steps for all 0 <= k <= n <= 32")
{
    for (int n = 0; n <= 32; ++n)
        for (int k = 0; k <= n; ++k)
        {
            CAPTURE (k, n);

            const auto pattern = bjorklund (k, n);
            REQUIRE (static_cast<int> (pattern.size ()) == n);
            REQUIRE (hitCount (pattern) == k);
        }
}

TEST_CASE ("bjorklund throws std::invalid_argument if the values are not valid")
{
    REQUIRE_THROWS_AS (bjorklund (-1, 8), std::invalid_argument);
    REQUIRE_THROWS_AS (bjorklund (9, 8), std::invalid_argument);
    REQUIRE_THROWS_AS (bjorklund (0, -1), std::invalid_argument);
}

TEST_CASE ("rotate moves each slot to the right for a positive value")
{
    const std::vector<int> pattern { 1, 0, 0, 1, 0 };

    REQUIRE (algo::rotate (pattern, 0) == pattern);
    REQUIRE (algo::rotate (pattern, 1) == std::vector<int> { 0, 1, 0, 0, 1 });
    REQUIRE (algo::rotate (pattern, 2) == std::vector<int> { 1, 0, 1, 0, 0 });
}

TEST_CASE ("rotate moves each slot to the left for a negative value")
{
    const std::vector<int> pattern { 1, 0, 0, 1, 0 };

    REQUIRE (algo::rotate (pattern, -1) == std::vector<int> { 0, 0, 1, 0, 1 });
    REQUIRE (algo::rotate (algo::rotate (pattern, 3), -3) == pattern);
}

TEST_CASE ("rotate wraps a value larger than the pattern")
{
    const std::vector<int> pattern { 1, 0, 0, 1, 0 };

    REQUIRE (algo::rotate (pattern, 5) == pattern);
    REQUIRE (algo::rotate (pattern, 6) == algo::rotate (pattern, 1));
    REQUIRE (algo::rotate (pattern, -6) == algo::rotate (pattern, -1));
}

TEST_CASE ("rotate accepts an empty pattern")
{
    REQUIRE (algo::rotate ({}, 3).empty ());
}

TEST_CASE ("rotate does not change the hit count")
{
    for (int n = 0; n <= 32; ++n)
        for (int k = 0; k <= n; ++k)
            for (int shift = -n - 1; shift <= n + 1; ++shift)
            {
                CAPTURE (k, n, shift);
                REQUIRE (hitCount (algo::rotate (bjorklund (k, n), shift)) == k);
            }
}

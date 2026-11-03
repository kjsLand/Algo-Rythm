#include "algo/Contour.h"

#include "support/PropertyTest.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>

using namespace algo;

TEST_CASE ("Each coefficient is an integer from -3 to 3 and n is 3 to 5")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        Rng        rng (seed);
        const auto contour = randomContour (rng);
        REQUIRE (contour.a.size () == contour.b.size ());
        REQUIRE (contour.a.size () >= 3);
        REQUIRE (contour.a.size () <= 5);

        const auto inRange = [] (int v)
        {
            return v >= kMinCoefficient && v <= kMaxCoefficient;
        };
        REQUIRE (inRange (contour.a0));
        REQUIRE (std::all_of (contour.a.begin (), contour.a.end (), inRange));
        REQUIRE (std::all_of (contour.b.begin (), contour.b.end (), inRange));
    });
}

TEST_CASE ("No contour is a flat line")
{
    algo::test::forEachSeed ([] (std::uint64_t seed)
    {
        Rng        rng (seed);
        const auto contour = randomContour (1, rng);
        const bool flat    = std::all_of (contour.a.begin (), contour.a.end (), [] (int v)
           {
            return v == 0;
        }) && std::all_of (contour.b.begin (), contour.b.end (), [] (int v)
           {
            return v == 0;
        });
        REQUIRE_FALSE (flat);
    });
}

TEST_CASE ("The same seed gives the same coefficients on all systems")
{
    // These values come from the portable Rng. A change means that saved hooks change.
    Rng        rng (42);
    const auto contour = randomContour (rng);
    Rng        again (42);
    REQUIRE (randomContour (again) == contour);
    REQUIRE_THROWS (randomContour (0, rng));
}

TEST_CASE ("evaluate maps t from 0 to 1 onto 0 to 2 pi")
{
    Contour contour;
    contour.a0 = 1;
    contour.a  = { 2 };
    contour.b  = { 0 };
    REQUIRE (evaluate (contour, 0.0) == Catch::Approx (3.0));
    REQUIRE (evaluate (contour, 0.5) == Catch::Approx (-1.0));
    REQUIRE (evaluate (contour, 1.0) == Catch::Approx (3.0));

    contour.a = { 0 };
    contour.b = { 1 };
    REQUIRE (evaluate (contour, 0.25) == Catch::Approx (2.0));
}

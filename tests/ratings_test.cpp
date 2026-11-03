#include "algo/Engine.h"
#include "algo/Ratings.h"

#include <catch2/catch_test_macros.hpp>

using namespace algo;

TEST_CASE ("A rating line has the date, seed, key, difficulty, rating, progressions and table type")
{
    GenerationSettings settings;
    settings.masterSeed = 81723;
    auto starter        = generate (settings);

    REQUIRE_THROWS_AS (ratingCsvLine (starter, "2026-11-03"), std::invalid_argument);
    starter.rating = 6;
    REQUIRE_THROWS_AS (ratingCsvLine (starter, "2026-11-03"), std::invalid_argument);

    starter.rating  = 5;
    const auto line = ratingCsvLine (starter, "2026-11-03", "learned");
    REQUIRE (line == "2026-11-03,81723,G,Major,Beginner,5,Verse: 1 2 1 5 | Chorus: 1 5 6m 4,learned");
    REQUIRE (std::string (kRatingsHeader) == "date,seed,key,scale,difficulty,rating,progressions,tables");
}

TEST_CASE ("A CSV field with a comma or a quote is quoted")
{
    REQUIRE (csvField ("plain") == "plain");
    REQUIRE (csvField ("a,b") == "\"a,b\"");
    REQUIRE (csvField ("say \"hi\"") == "\"say \"\"hi\"\"\"");
}

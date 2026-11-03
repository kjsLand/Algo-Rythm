#include "algo/Version.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE ("The core version text is 0.1.0")
{
    REQUIRE (algo::version () == "0.1.0");
}

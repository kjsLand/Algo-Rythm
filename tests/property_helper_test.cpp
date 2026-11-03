#include "support/PropertyTest.h"

#include <catch2/catch_test_macros.hpp>

#include <stdlib.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using algo::test::forEachSeed;
using algo::test::propertySeedCount;

namespace
{
    constexpr const char* SEEDS_VARIABLE = "ALGO_PROP_SEEDS";

    void setSeedsVariable (const char* value)
    {
#ifdef _WIN32
        _putenv_s (SEEDS_VARIABLE, value == nullptr ? "" : value);
#else
        if (value == nullptr)
            unsetenv (SEEDS_VARIABLE);
        else
            setenv (SEEDS_VARIABLE, value, 1);
#endif
    }

    // CI sets the variable for the whole test run, so each test puts the old value back.
    class ScopedSeedsVariable
    {
    public:
        explicit ScopedSeedsVariable (const char* value)
        {
            if (const char* const old = getenv (SEEDS_VARIABLE))
                oldValue = old;

            setSeedsVariable (value);
        }

        ~ScopedSeedsVariable ()
        {
            setSeedsVariable (oldValue ? oldValue->c_str () : nullptr);
        }

    private:
        std::optional<std::string> oldValue;
    };
}

TEST_CASE ("propertySeedCount returns 1000 if the variable is not set")
{
    const ScopedSeedsVariable variable (nullptr);

    REQUIRE (propertySeedCount () == 1000);
}

TEST_CASE ("propertySeedCount reads the variable")
{
    const ScopedSeedsVariable variable ("10000");

    REQUIRE (propertySeedCount () == 10000);
}

TEST_CASE ("propertySeedCount rejects a value that is not a positive integer")
{
    for (const char* value : { "0", "-5", "abc", "5x" })
    {
        CAPTURE (value);

        const ScopedSeedsVariable variable (value);

        REQUIRE_THROWS_AS (propertySeedCount (), std::invalid_argument);
    }
}

TEST_CASE ("forEachSeed calls the function with seeds 1 to 5 if the variable is 5")
{
    const ScopedSeedsVariable variable ("5");

    std::vector<std::uint64_t> seeds;
    forEachSeed ([&seeds] (std::uint64_t seed)
    {
        seeds.push_back (seed);
    });

    REQUIRE (seeds == std::vector<std::uint64_t> { 1, 2, 3, 4, 5 });
}

TEST_CASE ("forEachSeed runs the configured number of seeds")
{
    std::uint64_t calls = 0;
    forEachSeed ([&calls] (std::uint64_t)
    {
        ++calls;
    });

    REQUIRE (calls == propertySeedCount ());
}

// This test fails on purpose. Hidden by [.], it runs only through the
// property_failure_shows_seed entry in tests/CMakeLists.txt.
TEST_CASE ("A failed property shows its seed", "[.][property-failure-fixture]")
{
    const ScopedSeedsVariable variable ("5");

    forEachSeed ([] (std::uint64_t seed)
    {
        REQUIRE (seed != 3);
    });
}

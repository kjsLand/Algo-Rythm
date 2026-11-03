#pragma once

#include <catch2/catch_test_macros.hpp>

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace algo::test
{
    /** The number of seeds for each property test: ALGO_PROP_SEEDS, or 1000 if it is not set. */
    inline std::uint64_t propertySeedCount ()
    {
        const char* const value = std::getenv ("ALGO_PROP_SEEDS");

        if (value == nullptr)
            return 1000;

        // A bad value must not silently run zero seeds and pass.
        char*               end   = nullptr;
        const std::uint64_t count = std::strtoull (value, &end, 10);

        if (!std::isdigit (static_cast<unsigned char> (value[0])) || *end != '\0' || count == 0)
            throw std::invalid_argument ("ALGO_PROP_SEEDS must be a positive integer");

        return count;
    }

    /** Calls fn with seeds 1 to propertySeedCount(). A failure in fn reports its seed. */
    template<typename Fn>
    void forEachSeed (Fn&& fn)
    {
        const std::uint64_t count = propertySeedCount ();

        for (std::uint64_t seed = 1; seed <= count; ++seed)
        {
            INFO ("Property failed for seed " << seed);
            fn (seed);
        }
    }
}

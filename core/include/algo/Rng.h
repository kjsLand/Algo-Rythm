#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace algo
{
    // The only source of random values in the core. The C++ standard defines the output of
    // its engine, and each function here defines its own use of that output, so one seed
    // gives the same values on all systems.
    //
    // An Rng cannot be copied, because two copies would give the same values. A moved-from
    // Rng must not be used.
    class Rng
    {
    public:
        explicit Rng (std::uint64_t seed);
        ~Rng ();

        Rng (Rng&&) noexcept;
        Rng& operator= (Rng&&) noexcept;

        // Each value from lo to hi, inclusive, has the same probability.
        // Throws std::invalid_argument if lo > hi.
        int uniformInt (int lo, int hi);

        // A value in [0, 1) from the top 53 bits of one engine output.
        double uniformReal ();

        // True with probability p. A value of p <= 0 is never true and p >= 1 is always true.
        bool chance (double p);

        // Returns an index with a probability proportional to its weight.
        // An index with weight 0 is never returned. Throws std::invalid_argument if the list
        // has no positive weight, or if a weight is negative or not finite.
        std::size_t weightedIndex (const std::vector<double>& weights);

        // Fisher-Yates shuffle.
        template<typename T>
        void shuffle (std::vector<T>& items)
        {
            for (int i = static_cast<int> (items.size ()) - 1; i > 0; --i)
                std::swap (items[static_cast<std::size_t> (i)], items[static_cast<std::size_t> (uniformInt (0, i))]);
        }

    private:
        struct Engine; // Only core/src/rng.cpp can name the engine type. Refer to tools/check_banned.py.

        std::unique_ptr<Engine> m_engine;
    };
}

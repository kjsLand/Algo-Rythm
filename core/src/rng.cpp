#include "algo/Rng.h"

#include <cmath>
#include <random>
#include <stdexcept>

namespace algo
{
    struct Rng::Engine
    {
        explicit Engine (std::uint64_t seed)
            : generator (seed)
        {
        }

        std::mt19937_64 generator;
    };

    Rng::Rng (std::uint64_t seed)
        : m_engine (std::make_unique<Engine> (seed))
    {
    }

    Rng::~Rng ()                         = default;
    Rng::Rng (Rng&&) noexcept            = default;
    Rng& Rng::operator= (Rng&&) noexcept = default;

    int Rng::uniformInt (int lo, int hi)
    {
        if (lo > hi)
            throw std::invalid_argument ("uniformInt: require lo <= hi");

        const auto span = static_cast<std::uint64_t> (static_cast<std::int64_t> (hi) - lo) + 1;

        // 2^64 mod span. The values below this limit would make the low results more likely.
        const std::uint64_t limit = (std::uint64_t { 0 } - span) % span;

        std::uint64_t value = m_engine->generator ();
        while (value < limit)
            value = m_engine->generator ();

        return static_cast<int> (lo + static_cast<std::int64_t> (value % span));
    }

    double Rng::uniformReal ()
    {
        // The value and 2^-53 are both exact in a double, so no system rounds this result.
        const std::uint64_t top53Bits = m_engine->generator () >> 11;
        return static_cast<double> (top53Bits) * 0x1p-53;
    }

    bool Rng::chance (double p)
    {
        return uniformReal () < p;
    }

    std::size_t Rng::weightedIndex (const std::vector<double>& weights)
    {
        double      total    = 0.0;
        std::size_t lastUsed = 0;

        for (std::size_t i = 0; i < weights.size (); ++i)
        {
            if (!std::isfinite (weights[i]) || weights[i] < 0.0)
                throw std::invalid_argument ("weightedIndex: each weight must be finite and not negative");

            total += weights[i];

            if (weights[i] > 0.0)
                lastUsed = i;
        }

        if (!std::isfinite (total))
            throw std::invalid_argument ("weightedIndex: the total of the weights must be finite");

        if (total <= 0.0)
            throw std::invalid_argument ("weightedIndex: at least one weight must be more than 0");

        const double target = uniformReal () * total;

        double sum = 0.0;
        for (std::size_t i = 0; i < weights.size (); ++i)
        {
            sum += weights[i];

            if (target < sum)
                return i;
        }

        // Rounding can make the target equal to the last sum.
        return lastUsed;
    }
}

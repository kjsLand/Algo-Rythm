#include "algo/Euclid.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace algo
{
    std::vector<int> bjorklund (int hits, int steps)
    {
        if (hits < 0 || hits > steps)
            throw std::invalid_argument ("bjorklund: require 0 <= hits <= steps");

        const auto stepCount = static_cast<std::size_t> (steps);

        if (hits == 0)
            return std::vector<int> (stepCount, 0);
        if (hits == steps)
            return std::vector<int> (stepCount, 1);

        std::vector<std::vector<int>> pattern (static_cast<std::size_t> (hits), { 1 });
        std::vector<std::vector<int>> remainder (static_cast<std::size_t> (steps - hits), { 0 });

        while (remainder.size () > 1)
        {
            const std::size_t distribute = std::min (pattern.size (), remainder.size ());

            std::vector<std::vector<int>> newPattern;
            newPattern.reserve (distribute);

            for (std::size_t i = 0; i < distribute; ++i)
            {
                auto merged = pattern[i];
                merged.insert (merged.end (), remainder[i].begin (), remainder[i].end ());
                newPattern.push_back (std::move (merged));
            }

            const auto&                   longer = pattern.size () > remainder.size () ? pattern : remainder;
            std::vector<std::vector<int>> newRemainder (longer.begin () + static_cast<std::ptrdiff_t> (distribute), longer.end ());

            pattern   = std::move (newPattern);
            remainder = std::move (newRemainder);
        }

        std::vector<int> result;
        result.reserve (stepCount);
        for (const auto& group : pattern)
            result.insert (result.end (), group.begin (), group.end ());
        for (const auto& group : remainder)
            result.insert (result.end (), group.begin (), group.end ());
        return result;
    }

    std::vector<int> rotate (std::vector<int> pattern, int n)
    {
        if (pattern.empty ())
            return pattern;

        const auto size  = static_cast<int> (pattern.size ());
        const int  shift = ((n % size) + size) % size;
        std::rotate (pattern.begin (), pattern.end () - shift, pattern.end ());
        return pattern;
    }

    int hitCount (const std::vector<int>& pattern)
    {
        return static_cast<int> (std::count_if (pattern.begin (), pattern.end (), [] (int slot)
        {
            return slot != 0;
        }));
    }
}

#pragma once

#include <vector>
#include <stdexcept>
#include <algorithm>

//==============================================================================
/**
    Bjorklund / Euclidean rhythm algorithm.

    Distributes `hits` onsets as evenly as possible across `steps` slots.
    Returns a std::vector<int> of 1s (hit) and 0s (rest).

    Example:  bjorklund(3, 8) → { 1,0,0,1,0,0,1,0 }   (Tresillo)
*/
inline std::vector<int> bjorklund (int hits, int steps)
{
    if (hits < 0 || steps <= 0 || hits > steps)
        throw std::invalid_argument ("bjorklund: require 0 <= hits <= steps, steps > 0");

    if (hits == 0) return std::vector<int> (steps, 0);
    if (hits == steps) return std::vector<int> (steps, 1);

    std::vector<std::vector<int>> pattern (hits, { 1 });
    std::vector<std::vector<int>> remainder (steps - hits, { 0 });

    while (remainder.size () > 1)
    {
        int distribute = (int) std::min (pattern.size (), remainder.size ());

        std::vector<std::vector<int>> newPattern;
        newPattern.reserve (distribute);

        for (int i = 0; i < distribute; ++i)
        {
            auto merged = pattern[i];
            merged.insert (merged.end (), remainder[i].begin (), remainder[i].end ());
            newPattern.push_back (std::move (merged));
        }

        std::vector<std::vector<int>> newRemainder;
        if (pattern.size () > remainder.size ())
            newRemainder.assign (pattern.begin () + distribute, pattern.end ());
        else
            newRemainder.assign (remainder.begin () + distribute, remainder.end ());

        pattern   = std::move (newPattern);
        remainder = std::move (newRemainder);
    }

    std::vector<int> result;
    result.reserve (steps);
    for (auto& g : pattern) result.insert (result.end (), g.begin (), g.end ());
    for (auto& g : remainder) result.insert (result.end (), g.begin (), g.end ());
    return result;
}

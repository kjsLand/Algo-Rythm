#pragma once

#include <vector>

namespace algo
{
    // Bjorklund / Euclidean rhythm algorithm.
    //
    // Distributes `hits` onsets as evenly as possible across `steps` slots.
    // Returns 1 for a hit and 0 for a rest. Example: bjorklund (3, 8) gives { 1, 0, 0, 1, 0, 0, 1, 0 }.
    // Throws std::invalid_argument if the values are not in the range 0 <= hits <= steps.
    std::vector<int> bjorklund (int hits, int steps);

    // A positive n moves each slot to the right and a negative n moves each slot to the left.
    // The slots wrap around the end of the pattern.
    std::vector<int> rotate (std::vector<int> pattern, int n);

    int hitCount (const std::vector<int>& pattern);
}

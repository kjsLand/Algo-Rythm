#include "algo/Seeds.h"

namespace algo
{
    namespace
    {
        // The SplitMix64 mix function, with its increment.
        std::uint64_t splitMix64 (std::uint64_t value)
        {
            value += 0x9E3779B97F4A7C15ULL;
            value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
            value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
            return value ^ (value >> 31);
        }

        // The conversion from int is modular, so a negative value also gives the same result on all systems.
        std::uint64_t combine (std::uint64_t hash, int value)
        {
            return splitMix64 (hash ^ static_cast<std::uint64_t> (static_cast<std::int64_t> (value)));
        }
    }

    std::uint64_t deriveSeed (std::uint64_t master, Stage stage, int sectionIndex, int rerollCount)
    {
        std::uint64_t hash = splitMix64 (master);
        hash               = combine (hash, static_cast<int> (stage));
        hash               = combine (hash, sectionIndex);
        return combine (hash, rerollCount);
    }

    Rng rngFor (std::uint64_t master, Stage stage, int sectionIndex, int rerollCount)
    {
        return Rng (deriveSeed (master, stage, sectionIndex, rerollCount));
    }
}

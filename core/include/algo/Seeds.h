#pragma once

#include "Rng.h"

#include <cstdint>

namespace algo
{
    // These values go into each derived seed. A change to a value changes all saved results.
    enum class Stage
    {
        Plan    = 0,
        Chords  = 1,
        Voicing = 2,
        Strum   = 3,
        Hook    = 4,
        Render  = 5
    };

    // Gives each stage and section its own seed, so a reroll of one item cannot change the others.
    std::uint64_t deriveSeed (std::uint64_t master, Stage stage, int sectionIndex, int rerollCount);

    // Returns a new Rng with the seed from deriveSeed.
    Rng rngFor (std::uint64_t master, Stage stage, int sectionIndex, int rerollCount);
}

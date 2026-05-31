#include "StrumPattern.h"
#include "Bjorklund.h"

#include <algorithm>
#include <stdexcept>

//==============================================================================
int StrumPattern::stepsFromBpm (int bpm)
{
    // Fast tempos use 8th-note grid; everything else 16th-note grid.
    return (bpm >= 140) ? 8 : 16;
}

//==============================================================================
std::string StrumPattern::buildRuler (int steps)
{
    // Produces e.g. "1.+.2.+.3.+.4.+." for 16 steps
    //              or "1+2+3+4+" for 8 steps
    const int div = steps / 4;
    std::string ruler;
    ruler.reserve ((size_t) steps);

    for (int i = 0; i < steps; ++i)
    {
        if      (i % div == 0)           ruler += (char)('1' + i / div);
        else if (div >= 4 && i % 2 == 0) ruler += '+';
        else                             ruler += '.';
    }
    return ruler;
}

//==============================================================================
StrumPattern::Result StrumPattern::generate (int bpm, unsigned int seed)
{
    if (seed == 0)
    {
        std::random_device rd;
        seed = rd();
    }

    std::mt19937 rng (seed);

    Result r;
    r.bpm   = bpm;
    r.steps = stepsFromBpm (bpm);

    // ── Layer 1: strum hits (half to three-quarters of steps) ──────────────
    {
        const int minH = r.steps / 2;
        const int maxH = (r.steps * 3) / 4;
        std::uniform_int_distribution<int> dist (minH, maxH);
        r.strumHits = dist (rng);
    }
    r.strumBits = bjorklund (r.strumHits, r.steps);

    // ── Layer 2: rest mask (sparse – 1 to steps/4 rests) ───────────────────
    {
        std::uniform_int_distribution<int> dist (1, r.steps / 4);
        r.restHits = dist (rng);
    }
    r.restBits = bjorklund (r.restHits, r.steps);

    // ── Combine layers ──────────────────────────────────────────────────────
    // Direction ticks on every subdivision slot so the wrist never stops.
    // Slots silenced by the rest mask become '-'.
    // Slots where the strum layer is 0 also become '-'.
    std::string out;
    out.reserve ((size_t) r.steps);

    int dir = 0; // 0 = Down, 1 = Up
    for (int i = 0; i < r.steps; ++i)
    {
        char arrow = (dir % 2 == 0) ? 'D' : 'U';
        ++dir;

        if (r.restBits[i] == 1 || r.strumBits[i] == 0)
            out += '-';
        else
            out += arrow;
    }

    r.pattern = std::move (out);
    r.ruler   = buildRuler (r.steps);
    return r;
}

//==============================================================================
StrumPattern::Result StrumPattern::generateRandom (unsigned int seed)
{
    if (seed == 0)
    {
        std::random_device rd;
        seed = rd();
    }
    std::mt19937 rng (seed);
    std::uniform_int_distribution<int> bpmDist (60, 180);
    return generate (bpmDist (rng), seed);
}

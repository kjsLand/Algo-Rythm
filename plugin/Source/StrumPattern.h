#pragma once

#include "algo/Euclid.h"

#include <algorithm>
#include <string>
#include <vector>
#include <random>

//==============================================================================
/**
    Generates a guitar strum pattern for one bar of 4/4 using two Euclidean
    (Bjorklund) bitstrings layered together.  Header-only — no .cpp needed.

    Layer 1 – Strum grid
        A Bjorklund pattern whose 1s are stroked notes.
        Direction alternates D/U across every subdivision slot (whether hit or
        not), so the wrist keeps swinging at a constant rate.

    Layer 2 – Rest mask
        A second sparser Bjorklund pattern.  Any slot that is 1 in the mask is
        silenced in the final output regardless of the strum layer.

    Final output character key
        'D'  down-strum (audible)
        'U'  up-strum   (audible)
        '-'  rest / mute
*/
class StrumPattern
{
public:
    //==========================================================================
    struct Result
    {
        int              bpm { 120 };
        int              steps { 16 }; ///< subdivisions per bar
        int              strumHits { 0 };
        int              restHits { 0 };
        std::vector<int> strumBits;
        std::vector<int> restBits;
        std::string      pattern; ///< final D/U/- string

        /** Beat-grid ruler string aligned to pattern, e.g. "1.+.2.+.3.+.4.+." */
        std::string ruler;
    };

    //==========================================================================
    /** Generate a random strum pattern.
        @param bpm   Tempo in BPM — determines whether to use 8 or 16 steps.
        @param seed  RNG seed; pass 0 to pull from std::random_device.
    */
    static Result generate (int bpm, unsigned int seed = 0)
    {
        if (seed == 0)
        {
            std::random_device rd;
            seed = rd ();
        }

        std::mt19937 rng (seed);

        Result r;
        r.bpm   = bpm;
        r.steps = stepsFromBpm (bpm);

        // ── Layer 1: strum hits (half to three-quarters of steps) ───────────
        {
            const int                          minH = r.steps / 2;
            const int                          maxH = (r.steps * 3) / 4;
            std::uniform_int_distribution<int> dist (minH, maxH);
            r.strumHits = dist (rng);
        }
        r.strumBits = algo::bjorklund (r.strumHits, r.steps);

        // ── Layer 2: rest mask (1 to steps/4 rests) ─────────────────────────
        {
            std::uniform_int_distribution<int> dist (1, r.steps / 4);
            r.restHits = dist (rng);
        }
        r.restBits = algo::bjorklund (r.restHits, r.steps);

        // ── Keep beat 1 audible ─────────────────────────────────────────────
        // A Bjorklund pattern always starts with a 1, so the rest mask would
        // silence slot 0.  Rotation keeps the number of rests and their spacing.
        // restHits <= steps / 4, so the mask always contains a 0 and this ends.
        while (r.restBits[0] == 1)
            std::rotate (r.restBits.begin (), r.restBits.end () - 1, r.restBits.end ());

        r.strumBits[0] = 1;

        // ── Combine layers ───────────────────────────────────────────────────
        // Direction ticks on every slot; rest mask or strum=0 → '-'
        std::string out;
        out.reserve ((size_t) r.steps);

        int dir = 0;
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

    /** Convenience overload — picks a random BPM in [60, 180]. */
    static Result generateRandom (unsigned int seed = 0)
    {
        if (seed == 0)
        {
            std::random_device rd;
            seed = rd ();
        }
        std::mt19937                       rng (seed);
        std::uniform_int_distribution<int> bpmDist (60, 180);
        return generate (bpmDist (rng), seed);
    }

private:
    static int stepsFromBpm (int bpm)
    {
        return (bpm >= 140) ? 8 : 16;
    }

    static std::string buildRuler (int steps)
    {
        const int   div = steps / 4;
        std::string ruler;
        ruler.reserve ((size_t) steps);

        for (int i = 0; i < steps; ++i)
        {
            if (i % div == 0)
                ruler += (char) ('1' + i / div);
            else if (div >= 4 && i % 2 == 0)
                ruler += '+';
            else
                ruler += '.';
        }
        return ruler;
    }
};

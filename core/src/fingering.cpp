#include "algo/Voicing.h"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace algo
{
    namespace
    {
        std::size_t indexOf (int string)
        {
            return static_cast<std::size_t> (string);
        }

        bool isFretted (int fret)
        {
            return fret > 0;
        }
    }

    std::optional<Barre> findBarre (const Voicing& voicing)
    {
        int lowest = 0;
        for (int fret : voicing.frets)
            if (isFretted (fret) && (lowest == 0 || fret < lowest))
                lowest = fret;

        std::optional<Barre> best;
        int                  bestNotes = 1;

        // A run of strings with no open or muted string is the area that one finger can cover.
        for (int s = 0; s < kNumStrings; ++s)
        {
            Barre barre { lowest, -1, -1 };
            int   notes = 0;

            for (; s < kNumStrings && isFretted (voicing.frets[indexOf (s)]); ++s)
            {
                if (voicing.frets[indexOf (s)] != lowest)
                    continue;

                if (barre.firstString < 0)
                    barre.firstString = s;
                barre.lastString = s;
                ++notes;
            }

            if (notes > bestNotes)
            {
                best      = barre;
                bestNotes = notes;
            }
        }
        return best;
    }

    int fingerCount (const Voicing& voicing)
    {
        const auto barre = findBarre (voicing);

        int fingers = barre ? 1 : 0;
        for (int s = 0; s < kNumStrings; ++s)
        {
            const int  fret     = voicing.frets[indexOf (s)];
            const bool onTheBar = barre && fret == barre->fret && s >= barre->firstString && s <= barre->lastString;
            if (isFretted (fret) && !onTheBar)
                ++fingers;
        }
        return fingers;
    }

    std::optional<Voicing> assignFingers (const Voicing& voicing)
    {
        Voicing result = voicing;
        result.fingers.fill (0);

        int nextFinger = 1;

        if (const auto barre = findBarre (voicing))
        {
            for (int s = barre->firstString; s <= barre->lastString; ++s)
                if (voicing.frets[indexOf (s)] == barre->fret)
                    result.fingers[indexOf (s)] = 1;
            nextFinger = 2;
        }

        std::vector<int> others;
        for (int s = 0; s < kNumStrings; ++s)
            if (isFretted (voicing.frets[indexOf (s)]) && result.fingers[indexOf (s)] == 0)
                others.push_back (s);

        std::stable_sort (others.begin (), others.end (), [&voicing] (int a, int b)
        {
            return voicing.frets[indexOf (a)] < voicing.frets[indexOf (b)];
        });

        for (int s : others)
        {
            if (nextFinger > kMaxFingers)
                return std::nullopt;
            result.fingers[indexOf (s)] = nextFinger++;
        }
        return result;
    }
}

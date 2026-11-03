#pragma once

#include "Data.h"
#include "Rng.h"
#include "Settings.h"
#include "Types.h"

#include <string>
#include <vector>

namespace algo
{
    // The result of stage 1. The later stages use it and do not change it.
    struct SongPlan
    {
        Key                      key;
        int                      bpm { 120 };
        TimeSig                  timeSig;
        int                      capo { 0 };
        Difficulty               difficulty { Difficulty::Beginner };
        std::string              form { "Verse-Chorus" };
        std::vector<FormSection> sections;
        bool                     fingerpicking { false }; // The strum stage uses the fingerpicking patterns
    };

    // Returns the name of the scale for the mood.
    std::string moodScale (Mood mood);

    // Selects the key, scale, BPM, capo and form.
    // Throws std::invalid_argument with the first error from validate if the settings are not valid.
    SongPlan planSong (const GenerationSettings& settings, Rng& rng, const DataSet& data = bundledData ());
}

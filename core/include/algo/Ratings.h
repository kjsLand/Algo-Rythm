#pragma once

#include "Data.h"
#include "Types.h"

#include <string>

namespace algo
{
    // The first line of ratings.csv. tools/analyze_corpus.py and tools/ab_report.py read these columns.
    inline constexpr const char* kRatingsHeader = "date,seed,key,scale,difficulty,rating,progressions,tables";

    // The numerals of each section, for example "Verse: 1 5 6m 4 | Chorus: 4 1 5 6m".
    // Each section gives its first 4 chords (one loop).
    std::string progressionText (const SongStarter& starter);

    // One line of ratings.csv, with no line end. tables is empty, or "rules" or "learned" in A/B test mode.
    // Throws std::invalid_argument if the song starter has no rating from 1 to 5 (REQ-14-4).
    std::string ratingCsvLine (const SongStarter& starter, const std::string& date, const std::string& tables = {});

    // Quotes a CSV field if it has a comma, a quote or a line end.
    std::string csvField (const std::string& text);
}

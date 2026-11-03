#include "algo/Ratings.h"

#include "algo/Json.h"
#include "algo/Nashville.h"
#include "algo/Pitch.h"

#include <stdexcept>

namespace algo
{
    std::string csvField (const std::string& text)
    {
        if (text.find_first_of (",\"\r\n") == std::string::npos)
            return text;

        std::string quoted = "\"";
        for (char c : text)
            quoted += c == '"' ? std::string ("\"\"") : std::string (1, c);
        return quoted + "\"";
    }

    std::string progressionText (const SongStarter& starter)
    {
        std::string text;
        for (const auto& section : starter.sections)
        {
            if (!text.empty ())
                text += " | ";
            text += section.name + ":";

            std::size_t count = 0;
            for (const auto& bar : section.bars)
                for (const auto& chord : bar.chords)
                    if (count++ < 4)
                        text += " " + numeralText (chord);
        }
        return text;
    }

    std::string ratingCsvLine (const SongStarter& starter, const std::string& date, const std::string& tables)
    {
        if (!starter.rating || *starter.rating < kMinRating || *starter.rating > kMaxRating)
            throw std::invalid_argument ("A rating must be an integer from 1 to 5");

        return csvField (date) + "," + std::to_string (starter.seed) + "," + csvField (pitchClassName (starter.key.root, starter.key)) + "," + csvField (starter.key.scale) + ","
               + difficultyName (starter.difficulty) + "," + std::to_string (*starter.rating) + "," + csvField (progressionText (starter)) + "," + csvField (tables);
    }
}

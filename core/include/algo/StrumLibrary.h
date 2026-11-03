#pragma once

#include "Data.h"
#include "Rng.h"
#include "Types.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace algo
{
    // The strum patterns from strums.json. The entries stay in file order.
    class StrumLibrary
    {
    public:
        StrumLibrary () = default;
        explicit StrumLibrary (std::vector<StrumDefinition> definitions);

        // Uses the strums of the data set.
        static StrumLibrary fromData (const DataSet& data = bundledData ());

        const std::vector<StrumDefinition>& all () const
        {
            return m_definitions;
        }

        // Sets a weight for each pattern name. A pattern with no weight has the weight 1.0.
        // The learned tables (SPEC-15) use this. Throws std::invalid_argument if a weight is not positive.
        void setWeights (const std::map<std::string, double>& weights);

        double weight (const std::string& name) const;

        // Returns the patterns that have the difficulty or a lower difficulty, the step count and the hit count.
        std::vector<StrumDefinition> filter (Difficulty difficulty, int steps, int targetHits) const;

        // Chooses one pattern from filter with weightedIndex. Returns no value if no pattern passes the filter.
        std::optional<StrumDefinition> pickLibraryPattern (Difficulty difficulty, int steps, int targetHits, Rng& rng) const;

    private:
        std::vector<StrumDefinition>  m_definitions;
        std::map<std::string, double> m_weights;
    };
}

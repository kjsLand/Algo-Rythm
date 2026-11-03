#pragma once

#include "ChordRegistry.h"
#include "ProgressionLibrary.h"
#include "ScaleRegistry.h"
#include "Transitions.h"
#include "Types.h"

#include <array>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace algo
{
    struct StrumDefinition
    {
        std::string              name;
        StrumPattern             pattern;
        Difficulty               difficulty { Difficulty::Beginner };
        std::vector<std::string> tags;
    };

    // One chord shape. The frets go from the low E string to the high E string.
    //
    // A movable shape has frets relative to its root fret. The root string has the value 0.
    // Add the fret of the root note on the root string to each value.
    //
    // An open shape cannot move, so its root fret is 0 and its frets are the real frets.
    // The root of an open shape is the note at frets[rootStringIndex ()].
    struct ChordShape
    {
        std::string                                 name;
        std::string                                 chord;                        // A name in the chord registry
        int                                         rootString { 6 };             // Guitar number: 1 = high E, 6 = low E
        std::array<std::optional<int>, kNumStrings> frets;                        // No value = muted string
        std::array<int, kNumStrings>                fingers { 0, 0, 0, 0, 0, 0 }; // 0 = no finger
        bool                                        barre { false };              // One finger covers 2 or more strings
        bool                                        open { false };
        Difficulty                                  difficulty { Difficulty::Beginner };

        int rootStringIndex () const
        {
            return kNumStrings - rootString;
        }
    };

    struct FormSection
    {
        std::string name;
        std::string role; // "verse", "chorus", "bridge" or "loop"
        int         bars { 8 };
    };

    struct FormDefinition
    {
        std::string              name;
        std::vector<FormSection> sections;
    };

    // All data that the generator stages use.
    struct DataSet
    {
        ScaleRegistry                 scales;
        ChordRegistry                 chords;
        ProgressionLibrary            progressions;
        std::vector<StrumDefinition>  strums;
        std::vector<ChordShape>       shapes;
        std::vector<FormDefinition>   forms;
        TransitionTable               transitions;
        std::map<std::string, double> strumWeights; // Empty = each strum has the weight 1.0 (SPEC-15)
        std::vector<Tuning>           tunings;      // The first tuning is Standard
    };

    // Throw DataError if the text is not valid.
    std::vector<StrumDefinition> parseStrums (const std::string& text, const std::string& file = "strums.json");
    std::vector<ChordShape>      parseShapes (const std::string& text, const ChordRegistry& chords, const std::string& file = "shapes.json");
    std::vector<FormDefinition>  parseForms (const std::string& text, const std::string& file = "forms.json");
    std::vector<Tuning>          parseTunings (const std::string& text, const std::string& file = "tunings.json");

    // Returns the tuning with the name. Throws DataError if the data set has no such tuning.
    const Tuning& findTuning (const DataSet& data, const std::string& name);

    // Reads a learned strum weight file (SPEC-15): { "weights": { "<pattern name>": <weight> } }.
    std::map<std::string, double> parseStrumWeights (const std::string& text, const std::string& file = "strums.learned.json");

    // Loads the data files that are compiled into the binary.
    DataSet loadBundledData ();

    // The bundled data, loaded once. The theory functions use it if the caller gives no other data.
    const DataSet& bundledData ();

    // Loads the data files from a folder. If a file is not in the folder, the bundled file is used.
    // Throws DataError if the folder does not exist or a file is not valid.
    DataSet loadDataFolder (const std::filesystem::path& folder);
}

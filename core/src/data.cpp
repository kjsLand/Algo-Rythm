#include "algo/Data.h"

#include "algo/DataError.h"

#include "data_reader.h"
#include "embedded_data.h"

#include <fstream>
#include <functional>
#include <sstream>
#include <system_error>
#include <utility>

namespace algo
{
    namespace
    {
        std::string bundledText (const std::string& file)
        {
            for (std::size_t i = 0; i < detail::kEmbeddedFileCount; ++i)
            {
                const auto& embedded = detail::kEmbeddedFiles[i];
                if (file == embedded.name)
                    return std::string (reinterpret_cast<const char*> (embedded.bytes), embedded.size);
            }
            throw DataError (file, "", "the file is not in the bundled data");
        }

        DataSet loadAll (const std::function<std::string (const std::string&)>& read)
        {
            DataSet data;
            data.scales       = ScaleRegistry::fromJsonText (read ("scales.json"));
            data.chords       = ChordRegistry::fromJsonText (read ("chords.json"));
            data.progressions = ProgressionLibrary::fromJsonText (read ("progressions.json"), data.scales, data.chords);
            data.strums       = parseStrums (read ("strums.json"));
            data.shapes       = parseShapes (read ("shapes.json"), data.chords);
            return data;
        }
    }

    std::vector<StrumDefinition> parseStrums (const std::string& text, const std::string& file)
    {
        std::vector<StrumDefinition> strums;
        for (const auto& entry : detail::readEntries (text, file, "strums"))
        {
            StrumDefinition def;
            def.name           = entry.name ();
            def.difficulty     = entry.getDifficulty ("difficulty");
            def.tags           = entry.getStringList ("tags");
            def.pattern.steps  = entry.getInt ("steps");
            def.pattern.accent = entry.getNumberList ("accents");
            const auto slots   = entry.getString ("slots");

            if (static_cast<int> (slots.size ()) != def.pattern.steps)
                entry.fail ("'slots' has " + std::to_string (slots.size ()) + " characters, but 'steps' is " + std::to_string (def.pattern.steps));
            if (static_cast<int> (def.pattern.accent.size ()) != def.pattern.steps)
                entry.fail ("'accents' has " + std::to_string (def.pattern.accent.size ()) + " values, but 'steps' is " + std::to_string (def.pattern.steps));

            def.pattern.slots.clear ();
            for (char c : slots)
            {
                const auto stroke = parseStroke (c);
                if (!stroke)
                    entry.fail ("'slots' has the unknown stroke '" + std::string (1, c) + "'");
                def.pattern.slots.push_back (*stroke);
            }
            if (def.pattern.slots.empty () || def.pattern.slots[0] != Stroke::Down)
                entry.fail ("slot 0 must be 'D'");

            for (double accent : def.pattern.accent)
                if (accent < 0.0 || accent > 1.0)
                    entry.fail ("each accent must be in the range 0 to 1");

            strums.push_back (std::move (def));
        }
        return strums;
    }

    std::vector<ChordShape> parseShapes (const std::string& text, const ChordRegistry& chords, const std::string& file)
    {
        std::vector<ChordShape> shapes;
        for (const auto& entry : detail::readEntries (text, file, "shapes"))
        {
            ChordShape shape;
            shape.name       = entry.name ();
            shape.chord      = entry.getString ("chord");
            shape.rootString = entry.getInt ("rootString");
            shape.barre      = entry.getBool ("barre");
            shape.open       = entry.getBoolOr ("open", false);
            shape.difficulty = entry.getDifficulty ("difficulty");

            if (!chords.contains (shape.chord))
                entry.fail ("the chord '" + shape.chord + "' is not in the chord registry");
            if (shape.rootString < 1 || shape.rootString > kNumStrings)
                entry.fail ("'rootString' must be in the range 1 to 6");

            const Json& frets   = entry.getList ("frets");
            const auto  fingers = entry.getIntList ("fingers");
            if (frets.size () != kNumStrings)
                entry.fail ("'frets' must have 6 values");
            if (fingers.size () != kNumStrings)
                entry.fail ("'fingers' must have 6 values");

            for (std::size_t i = 0; i < kNumStrings; ++i)
            {
                if (frets[i].is_number_integer ())
                    shape.frets[i] = frets[i].get<int> ();
                else if (!frets[i].is_string () || frets[i].get<std::string> () != "x")
                    entry.fail ("each value in 'frets' must be an integer or \"x\"");

                if (fingers[i] < 0 || fingers[i] > 4)
                    entry.fail ("each value in 'fingers' must be in the range 0 to 4");
                shape.fingers[i] = fingers[i];
            }

            const auto& rootFret = shape.frets[static_cast<std::size_t> (shape.rootStringIndex ())];
            if (!rootFret)
                entry.fail ("the root string must not be muted");
            if (!shape.open && *rootFret != 0)
                entry.fail ("the root string of a movable shape must have the value 0");

            shapes.push_back (std::move (shape));
        }
        return shapes;
    }

    DataSet loadBundledData ()
    {
        return loadAll (bundledText);
    }

    const DataSet& bundledData ()
    {
        static const DataSet data = loadBundledData ();
        return data;
    }

    DataSet loadDataFolder (const std::filesystem::path& folder)
    {
        std::error_code error;
        if (!std::filesystem::is_directory (folder, error))
            throw DataError (folder.string (), "", "the data folder does not exist");

        return loadAll ([&folder] (const std::string& file)
        {
            const auto      path = folder / file;
            std::error_code fileError;
            if (!std::filesystem::exists (path, fileError))
                return bundledText (file);

            std::ifstream stream (path, std::ios::binary);
            if (!stream)
                throw DataError (file, "", "the file cannot be read");

            std::ostringstream text;
            text << stream.rdbuf ();
            return text.str ();
        });
    }
}

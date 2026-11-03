#include "algo/Data.h"

#include "algo/DataError.h"
#include "algo/Pitch.h"

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
            data.forms        = parseForms (read ("forms.json"));
            data.transitions  = TransitionTable::fromJsonText (read ("transitions.json"));
            data.tunings      = parseTunings (read ("tunings.json"));
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
            def.pattern.name   = def.name;
            def.pattern.steps  = entry.getInt ("steps");
            def.pattern.accent = entry.getNumberList ("accents");
            const auto slots   = entry.getString ("slots");

            if (static_cast<int> (slots.size ()) != def.pattern.steps)
                entry.fail ("'slots' has " + std::to_string (slots.size ()) + " characters, but 'steps' is " + std::to_string (def.pattern.steps));
            if (static_cast<int> (def.pattern.accent.size ()) != def.pattern.steps)
                entry.fail ("'accents' has " + std::to_string (def.pattern.accent.size ()) + " values, but 'steps' is " + std::to_string (def.pattern.steps));

            if (const char bad = setSlots (def.pattern, slots))
                entry.fail ("'slots' has the unknown stroke '" + std::string (1, bad) + "'");

            // A fingerpicking pattern starts with the bass string. A strum starts with a Down stroke.
            const bool picking = !def.pattern.pluck.empty ();
            if (def.pattern.slots.empty () || (picking ? def.pattern.slots[0] != Stroke::Pluck : def.pattern.slots[0] != Stroke::Down))
                entry.fail (picking ? "slot 0 of a fingerpicking pattern must be a Pluck" : "slot 0 must be 'D'");

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

    std::vector<FormDefinition> parseForms (const std::string& text, const std::string& file)
    {
        std::vector<FormDefinition> forms;
        for (const auto& entry : detail::readEntries (text, file, "forms"))
        {
            FormDefinition form;
            form.name = entry.name ();

            for (const Json& item : entry.getList ("sections"))
            {
                if (!item.is_object ())
                    entry.fail ("each value in 'sections' must be an object");

                const detail::EntryReader reader (item, file, entry.name ());
                FormSection               section { reader.getString ("name"), reader.getString ("role"), reader.getInt ("bars") };

                if (!detail::isSectionRole (section.role))
                    entry.fail ("the role '" + section.role + "' is not valid");
                if (section.bars <= 0)
                    entry.fail ("the field 'bars' must be larger than 0");

                form.sections.push_back (std::move (section));
            }

            if (form.sections.empty ())
                entry.fail ("the field 'sections' must not be empty");

            forms.push_back (std::move (form));
        }
        return forms;
    }

    std::vector<Tuning> parseTunings (const std::string& text, const std::string& file)
    {
        std::vector<Tuning> tunings;
        for (const auto& entry : detail::readEntries (text, file, "tunings"))
        {
            Tuning tuning;
            tuning.name        = entry.name ();
            const auto strings = entry.getStringList ("strings");
            if (strings.size () != kNumStrings)
                entry.fail ("'strings' must have 6 values");

            for (std::size_t i = 0; i < kNumStrings; ++i)
            {
                const auto note = tryParseNoteName (strings[i]);
                if (!note || !note->midiNote)
                    entry.fail ("'" + strings[i] + "' is not a note name with an octave number");
                tuning.strings[i] = *note->midiNote;
                if (i > 0 && tuning.strings[i] <= tuning.strings[i - 1])
                    entry.fail ("each string must be higher than the string before it");
            }
            tunings.push_back (std::move (tuning));
        }
        if (tunings.empty () || tunings.front ().name != "Standard")
            throw DataError (file, "", "the first tuning must be 'Standard'");
        return tunings;
    }

    const Tuning& findTuning (const DataSet& data, const std::string& name)
    {
        for (const auto& tuning : data.tunings)
            if (tuning.name == name)
                return tuning;
        throw DataError ("tunings.json", name, "the tuning is not in the data");
    }

    std::map<std::string, double> parseStrumWeights (const std::string& text, const std::string& file)
    {
        Json document;
        try
        {
            document = Json::parse (text);
        }
        catch (const Json::parse_error& error)
        {
            throw DataError (file, "", std::string ("the file is not valid JSON: ") + error.what ());
        }

        if (!document.is_object () || !document.contains ("weights") || !document.at ("weights").is_object ())
            throw DataError (file, "", "the field 'weights' must be an object");

        std::map<std::string, double> weights;
        for (const auto& item : document.at ("weights").items ())
        {
            if (!item.value ().is_number () || !(item.value ().get<double> () > 0.0))
                throw DataError (file, item.key (), "the weight must be a positive number");
            weights[item.key ()] = item.value ().get<double> ();
        }
        return weights;
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

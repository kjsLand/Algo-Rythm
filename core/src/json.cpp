#include "algo/Json.h"

#include <stdexcept>

namespace algo
{
    namespace
    {
        // The library converter for std::array ignores extra values. That would lose data.
        template<typename T>
        void readStringValues (const Json& j, const char* key, std::array<T, kNumStrings>& out)
        {
            const Json& values = j.at (key);
            if (!values.is_array () || values.size () != out.size ())
                throw std::invalid_argument (std::string (key) + " must have " + std::to_string (out.size ()) + " values");

            values.get_to (out);
        }
    }

    std::string difficultyName (Difficulty difficulty)
    {
        switch (difficulty)
        {
            case Difficulty::Beginner: return "Beginner";
            case Difficulty::Intermediate: return "Intermediate";
            case Difficulty::Advanced: return "Advanced";
        }
        throw std::invalid_argument ("Unknown difficulty value");
    }

    std::optional<Difficulty> parseDifficulty (const std::string& name)
    {
        for (auto difficulty : { Difficulty::Beginner, Difficulty::Intermediate, Difficulty::Advanced })
            if (difficultyName (difficulty) == name)
                return difficulty;
        return std::nullopt;
    }

    char strokeChar (Stroke stroke)
    {
        switch (stroke)
        {
            case Stroke::Down: return 'D';
            case Stroke::Up: return 'U';
            case Stroke::Rest: return '-';
            case Stroke::Mute: return 'x';
            case Stroke::Chuck: return 'c';
        }
        throw std::invalid_argument ("Unknown stroke value");
    }

    std::optional<Stroke> parseStroke (char c)
    {
        for (auto stroke : { Stroke::Down, Stroke::Up, Stroke::Rest, Stroke::Mute, Stroke::Chuck })
            if (strokeChar (stroke) == c)
                return stroke;
        return std::nullopt;
    }

    void to_json (Json& j, Difficulty v)
    {
        j = difficultyName (v);
    }

    void from_json (const Json& j, Difficulty& v)
    {
        const auto name       = j.get<std::string> ();
        const auto difficulty = parseDifficulty (name);
        if (!difficulty)
            throw std::invalid_argument ("Unknown difficulty: " + name);
        v = *difficulty;
    }

    void to_json (Json& j, Stroke v)
    {
        j = std::string (1, strokeChar (v));
    }

    void from_json (const Json& j, Stroke& v)
    {
        const auto text   = j.get<std::string> ();
        const auto stroke = text.size () == 1 ? parseStroke (text[0]) : std::nullopt;
        if (!stroke)
            throw std::invalid_argument ("Unknown stroke: " + text);
        v = *stroke;
    }

    void to_json (Json& j, const Key& v)
    {
        j = Json { { "root", v.root }, { "scale", v.scale } };
    }

    void from_json (const Json& j, Key& v)
    {
        j.at ("root").get_to (v.root);
        j.at ("scale").get_to (v.scale);
    }

    void to_json (Json& j, const TimeSig& v)
    {
        j = Json { { "numerator", v.numerator }, { "denominator", v.denominator } };
    }

    void from_json (const Json& j, TimeSig& v)
    {
        j.at ("numerator").get_to (v.numerator);
        j.at ("denominator").get_to (v.denominator);
    }

    void to_json (Json& j, const ChordSymbol& v)
    {
        j = Json { { "degree", v.degree },
                   { "accidental", v.accidental },
                   { "quality", v.quality },
                   { "bassDegree", v.bassDegree },
                   { "bassAccidental", v.bassAccidental },
                   { "tags", v.tags } };
    }

    void from_json (const Json& j, ChordSymbol& v)
    {
        j.at ("degree").get_to (v.degree);
        j.at ("accidental").get_to (v.accidental);
        j.at ("quality").get_to (v.quality);
        j.at ("bassDegree").get_to (v.bassDegree);
        j.at ("bassAccidental").get_to (v.bassAccidental);
        j.at ("tags").get_to (v.tags);
    }

    void to_json (Json& j, const FretPos& v)
    {
        j = Json { { "string", v.string }, { "fret", v.fret } };
    }

    void from_json (const Json& j, FretPos& v)
    {
        j.at ("string").get_to (v.string);
        j.at ("fret").get_to (v.fret);
    }

    void to_json (Json& j, const Voicing& v)
    {
        j = Json { { "frets", v.frets }, { "fingers", v.fingers } };
    }

    void from_json (const Json& j, Voicing& v)
    {
        readStringValues (j, "frets", v.frets);
        readStringValues (j, "fingers", v.fingers);
    }

    void to_json (Json& j, const StrumPattern& v)
    {
        std::string slots;
        for (auto stroke : v.slots)
            slots += strokeChar (stroke);

        j = Json { { "steps", v.steps }, { "slots", slots }, { "accent", v.accent }, { "strings", v.strings } };
    }

    void from_json (const Json& j, StrumPattern& v)
    {
        j.at ("steps").get_to (v.steps);
        j.at ("accent").get_to (v.accent);
        j.at ("strings").get_to (v.strings);

        v.slots.clear ();
        for (char c : j.at ("slots").get<std::string> ())
        {
            const auto stroke = parseStroke (c);
            if (!stroke)
                throw std::invalid_argument ("Unknown stroke: " + std::string (1, c));
            v.slots.push_back (*stroke);
        }

        if (static_cast<int> (v.slots.size ()) != v.steps || static_cast<int> (v.accent.size ()) != v.steps)
            throw std::invalid_argument ("slots and accent must each have " + std::to_string (v.steps) + " values");
    }

    void to_json (Json& j, const MelodyNote& v)
    {
        j = Json { { "pitch", v.pitch }, { "start", v.start }, { "length", v.length }, { "velocity", v.velocity } };
    }

    void from_json (const Json& j, MelodyNote& v)
    {
        j.at ("pitch").get_to (v.pitch);
        j.at ("start").get_to (v.start);
        j.at ("length").get_to (v.length);
        j.at ("velocity").get_to (v.velocity);
    }

    void to_json (Json& j, const Bar& v)
    {
        j = Json { { "chords", v.chords }, { "voicings", v.voicings } };
    }

    void from_json (const Json& j, Bar& v)
    {
        j.at ("chords").get_to (v.chords);
        j.at ("voicings").get_to (v.voicings);
    }

    void to_json (Json& j, const Section& v)
    {
        j = Json { { "name", v.name }, { "bars", v.bars }, { "strum", v.strum } };
    }

    void from_json (const Json& j, Section& v)
    {
        j.at ("name").get_to (v.name);
        j.at ("bars").get_to (v.bars);
        j.at ("strum").get_to (v.strum);
    }

    void to_json (Json& j, const Tuning& v)
    {
        j = Json { { "name", v.name }, { "strings", v.strings } };
    }

    void from_json (const Json& j, Tuning& v)
    {
        j.at ("name").get_to (v.name);
        readStringValues (j, "strings", v.strings);
    }

    void to_json (Json& j, const SongStarter& v)
    {
        j = Json { { "schemaVersion", v.schemaVersion },
                   { "seed", v.seed },
                   { "key", v.key },
                   { "bpm", v.bpm },
                   { "timeSig", v.timeSig },
                   { "capo", v.capo },
                   { "tuning", v.tuning },
                   { "difficulty", v.difficulty },
                   { "form", v.form },
                   { "sections", v.sections },
                   { "hook", v.hook } };
    }

    void from_json (const Json& j, SongStarter& v)
    {
        if (!j.is_object () || !j.contains ("schemaVersion"))
            throw std::runtime_error ("The song starter has no schemaVersion");

        const int version = j.at ("schemaVersion").get<int> ();
        if (version > kSchemaVersion)
            throw std::runtime_error ("The song starter has schemaVersion " + std::to_string (version) + ". The largest supported version is " + std::to_string (kSchemaVersion) + ".");

        v.schemaVersion = version;
        j.at ("seed").get_to (v.seed);
        j.at ("key").get_to (v.key);
        j.at ("bpm").get_to (v.bpm);
        j.at ("timeSig").get_to (v.timeSig);
        j.at ("capo").get_to (v.capo);
        j.at ("tuning").get_to (v.tuning);
        j.at ("difficulty").get_to (v.difficulty);
        j.at ("form").get_to (v.form);
        j.at ("sections").get_to (v.sections);
        j.at ("hook").get_to (v.hook);
    }
}

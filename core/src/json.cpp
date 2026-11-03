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
            case Stroke::Pluck: throw std::invalid_argument ("A Pluck has the character of its string. Use slotChar.");
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

    int pluckAt (const StrumPattern& pattern, std::size_t slot)
    {
        return slot < pattern.pluck.size () ? pattern.pluck[slot] : kPluckNone;
    }

    char slotChar (const StrumPattern& pattern, std::size_t slot)
    {
        const auto stroke = pattern.slots.at (slot);
        if (stroke != Stroke::Pluck)
            return strokeChar (stroke);

        const int string = pluckAt (pattern, slot);
        if (string == kPluckBass)
            return 'b';
        if (string == kPluckAltBass)
            return 'a';
        if (string >= 1 && string <= kNumStrings)
            return static_cast<char> ('0' + string);
        throw std::invalid_argument ("A Pluck slot has no string");
    }

    bool parseSlotChar (char c, Stroke& stroke, int& pluck)
    {
        pluck = kPluckNone;
        if (const auto parsed = parseStroke (c))
        {
            stroke = *parsed;
            return true;
        }
        stroke = Stroke::Pluck;
        if (c == 'b')
            pluck = kPluckBass;
        else if (c == 'a')
            pluck = kPluckAltBass;
        else if (c >= '1' && c <= '0' + kNumStrings)
            pluck = c - '0';
        else
            return false;
        return true;
    }

    char setSlots (StrumPattern& pattern, const std::string& text)
    {
        pattern.slots.clear ();
        pattern.pluck.clear ();
        bool             anyPluck = false;
        std::vector<int> plucks;
        for (char c : text)
        {
            Stroke stroke = Stroke::Rest;
            int    pluck  = kPluckNone;
            if (!parseSlotChar (c, stroke, pluck))
                return c;
            pattern.slots.push_back (stroke);
            plucks.push_back (pluck);
            anyPluck = anyPluck || stroke == Stroke::Pluck;
        }
        if (anyPluck)
            pattern.pluck = plucks;
        return 0;
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
        for (std::size_t i = 0; i < v.slots.size (); ++i)
            slots += slotChar (v, i);

        j = Json { { "steps", v.steps }, { "slots", slots }, { "accent", v.accent }, { "strings", v.strings } };
        if (!v.name.empty ())
            j["name"] = v.name;
    }

    void from_json (const Json& j, StrumPattern& v)
    {
        j.at ("steps").get_to (v.steps);
        j.at ("accent").get_to (v.accent);
        j.at ("strings").get_to (v.strings);
        v.name = j.value ("name", std::string {});

        if (const char bad = setSlots (v, j.at ("slots").get<std::string> ()))
            throw std::invalid_argument ("Unknown stroke: " + std::string (1, bad));

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
        j = Json { { "name", v.name }, { "role", v.role }, { "bars", v.bars }, { "strum", v.strum } };
    }

    void from_json (const Json& j, Section& v)
    {
        j.at ("name").get_to (v.name);
        v.role = j.value ("role", std::string {});
        j.at ("bars").get_to (v.bars);
        j.at ("strum").get_to (v.strum);
    }

    void to_json (Json& j, const RerollCounts& v)
    {
        j = Json { { "chords", v.chords }, { "voicings", v.voicings }, { "strums", v.strums }, { "hook", v.hook } };
    }

    void from_json (const Json& j, RerollCounts& v)
    {
        v.chords   = j.value ("chords", std::vector<int> {});
        v.voicings = j.value ("voicings", std::vector<int> {});
        v.strums   = j.value ("strums", std::vector<int> {});
        v.hook     = j.value ("hook", 0);
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
                   { "hook", v.hook },
                   { "rerolls", v.rerolls },
                   { "tables", v.tables } };
        if (v.rating)
            j["rating"] = *v.rating;
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

        // These fields are newer than schema version 1, so a file can omit them.
        v.rerolls = j.contains ("rerolls") ? j.at ("rerolls").get<RerollCounts> () : RerollCounts {};
        v.tables  = j.value ("tables", std::string ("rules"));
        v.rating.reset ();
        if (j.contains ("rating") && !j.at ("rating").is_null ())
        {
            const int rating = j.at ("rating").get<int> ();
            if (rating < kMinRating || rating > kMaxRating)
                throw std::invalid_argument ("The rating must be 1 to 5: " + std::to_string (rating));
            v.rating = rating;
        }
    }
}

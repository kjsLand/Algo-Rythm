#include "algo/Settings.h"

#include "algo/Fretboard.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace algo
{
    namespace
    {
        // The chord stage builds chords from scale degrees 1 to 7.
        constexpr std::size_t kChordScaleNotes = 7;

        bool isBpmInRange (int bpm)
        {
            return bpm >= kMinBpm && bpm <= kMaxBpm;
        }

        std::string rangeText (int lo, int hi)
        {
            return std::to_string (lo) + " to " + std::to_string (hi);
        }
    }

    std::string moodName (Mood mood)
    {
        switch (mood)
        {
            case Mood::Bright: return "Bright";
            case Mood::Sad: return "Sad";
            case Mood::Hopeful: return "Hopeful";
            case Mood::Bluesy: return "Bluesy";
        }
        throw std::invalid_argument ("Unknown mood value");
    }

    std::optional<Mood> parseMood (const std::string& name)
    {
        for (auto mood : { Mood::Bright, Mood::Sad, Mood::Hopeful, Mood::Bluesy })
            if (moodName (mood) == name)
                return mood;
        return std::nullopt;
    }

    std::vector<std::string> validate (const GenerationSettings& settings, const DataSet& data)
    {
        std::vector<std::string> errors;

        if (settings.keyRoot && (*settings.keyRoot < 0 || *settings.keyRoot > 11))
            errors.push_back ("The key root must be " + rangeText (0, 11) + ", or any: " + std::to_string (*settings.keyRoot));

        if (settings.scale && !data.scales.contains (*settings.scale))
            errors.push_back ("Unknown scale: " + *settings.scale);
        else if (settings.scale && data.scales.get (*settings.scale).steps.size () != kChordScaleNotes)
            errors.push_back ("The scale must have 7 notes: " + *settings.scale);

        if (!isBpmInRange (settings.bpmMin) || !isBpmInRange (settings.bpmMax))
            errors.push_back ("The BPM range must be in " + rangeText (kMinBpm, kMaxBpm) + ": " + rangeText (settings.bpmMin, settings.bpmMax));

        if (settings.bpmMin > settings.bpmMax)
            errors.push_back ("The BPM minimum must not be more than the maximum: " + rangeText (settings.bpmMin, settings.bpmMax));

        // The plan stage always gives an even BPM, so a range must contain an even number.
        if (settings.bpmMin == settings.bpmMax && settings.bpmMin % 2 != 0)
            errors.push_back ("The BPM range must contain an even number: " + rangeText (settings.bpmMin, settings.bpmMax));

        const auto isForm = [&settings] (const FormDefinition& form)
        {
            return form.name == settings.form;
        };
        if (std::none_of (data.forms.begin (), data.forms.end (), isForm))
            errors.push_back ("Unknown form: " + settings.form);

        if (settings.capo < 0 || settings.capo > Fretboard::kMaxCapo)
            errors.push_back ("The capo must be " + rangeText (0, Fretboard::kMaxCapo) + ": " + std::to_string (settings.capo));

        // T16-06 adds 6/8.
        if (!(settings.timeSig == TimeSig { 4, 4 }))
            errors.push_back ("The time signature must be 4/4: " + std::to_string (settings.timeSig.numerator) + "/" + std::to_string (settings.timeSig.denominator));

        return errors;
    }

    void to_json (Json& j, Mood v)
    {
        j = moodName (v);
    }

    void from_json (const Json& j, Mood& v)
    {
        const auto name = j.get<std::string> ();
        const auto mood = parseMood (name);
        if (!mood)
            throw std::invalid_argument ("Unknown mood: " + name);
        v = *mood;
    }

    void to_json (Json& j, const SectionLocks& v)
    {
        j = Json { { "chords", v.chords }, { "voicings", v.voicings }, { "strum", v.strum } };
    }

    void from_json (const Json& j, SectionLocks& v)
    {
        j.at ("chords").get_to (v.chords);
        j.at ("voicings").get_to (v.voicings);
        j.at ("strum").get_to (v.strum);
    }

    void to_json (Json& j, const Locks& v)
    {
        j = Json { { "key", v.key }, { "scale", v.scale }, { "tempo", v.tempo }, { "form", v.form }, { "sections", v.sections } };
    }

    void from_json (const Json& j, Locks& v)
    {
        j.at ("key").get_to (v.key);
        j.at ("scale").get_to (v.scale);
        j.at ("tempo").get_to (v.tempo);
        j.at ("form").get_to (v.form);
        j.at ("sections").get_to (v.sections);
    }

    void to_json (Json& j, const GenerationSettings& v)
    {
        j = Json { { "key", v.keyRoot ? Json (*v.keyRoot) : Json ("any") },
                   { "scale", v.scale ? Json (*v.scale) : Json (nullptr) },
                   { "mood", v.mood },
                   { "bpmMin", v.bpmMin },
                   { "bpmMax", v.bpmMax },
                   { "form", v.form },
                   { "difficulty", v.difficulty },
                   { "capo", v.capo },
                   { "timeSig", v.timeSig },
                   { "masterSeed", v.masterSeed },
                   { "locks", v.locks } };
    }

    void from_json (const Json& j, GenerationSettings& v)
    {
        const Json& key = j.at ("key");
        if (key.is_string () && key.get<std::string> () != "any")
            throw std::invalid_argument ("The key must be a root from 0 to 11, or any: " + key.get<std::string> ());
        v.keyRoot = key.is_string () ? std::nullopt : std::optional<PitchClass> (key.get<PitchClass> ());

        const Json& scale = j.at ("scale");
        v.scale           = scale.is_null () ? std::nullopt : std::optional<std::string> (scale.get<std::string> ());

        j.at ("mood").get_to (v.mood);
        j.at ("bpmMin").get_to (v.bpmMin);
        j.at ("bpmMax").get_to (v.bpmMax);
        j.at ("form").get_to (v.form);
        j.at ("difficulty").get_to (v.difficulty);
        j.at ("capo").get_to (v.capo);
        j.at ("timeSig").get_to (v.timeSig);
        j.at ("masterSeed").get_to (v.masterSeed);
        j.at ("locks").get_to (v.locks);
    }
}

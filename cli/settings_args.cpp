#include "SettingsArgs.h"

#include "algo/Engine.h"
#include "algo/Json.h"
#include "algo/Pitch.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <sstream>

namespace algo::cli
{
    namespace
    {
        std::string lower (std::string text)
        {
            std::transform (text.begin (), text.end (), text.begin (), [] (unsigned char c)
            {
                return static_cast<char> (std::tolower (c));
            });
            return text;
        }

        bool isNumber (const std::string& text)
        {
            return !text.empty () && std::all_of (text.begin (), text.end (), [] (unsigned char c)
            {
                return std::isdigit (c) != 0;
            });
        }
    }

    void SettingsArgs::addTo (CLI::App& command)
    {
        command.add_option ("--key", key, "The key root, for example G, F# or Bb, or \"any\"")->capture_default_str ();
        command.add_option ("--scale", scale, "The scale name, for example \"Major\" or \"Dorian\". The default comes from the mood.");
        command.add_option ("--mood", mood, "bright, sad, hopeful or bluesy")->capture_default_str ();
        command.add_option ("--bpm", bpm, "A BPM range such as 90-110, or one BPM such as 96")->capture_default_str ();
        command.add_option ("--form", form, "vc (Verse-Chorus), vcb (Verse-Chorus-Bridge) or loop")->capture_default_str ();
        command.add_option ("--difficulty", difficulty, "beginner, intermediate or advanced")->capture_default_str ();
        command.add_option ("--capo", capo, "The capo fret, 0 to 7")->capture_default_str ();
        command.add_option ("--time", time, "The time signature: 4/4 or 6/8")->capture_default_str ();
        command.add_option ("--tuning", tuning, "Standard, \"Drop D\", DADGAD, \"Open G\", \"Open D\" or \"Half-step down\"")->capture_default_str ();
        command.add_option ("--tables", tables, "rules or learned (SPEC-15)")->capture_default_str ();
        command.add_flag ("--hook", hook, "Add a melody hook of 2 to 4 bars");
        command.add_option ("--out", out, "The output folder")->capture_default_str ();
        command.add_option ("--formats", formats, "A comma-separated list of json, mid, tab and musicxml")->capture_default_str ();
    }

    std::vector<std::string> SettingsArgs::formatList () const
    {
        std::vector<std::string> list;
        std::stringstream        stream (formats);
        std::string              item;
        while (std::getline (stream, item, ','))
        {
            item = lower (item);
            if (item.empty ())
                continue;
            if (item != "json" && item != "mid" && item != "tab" && item != "musicxml")
                throw UsageError ("Unknown format: " + item + ". Use json, mid, tab or musicxml.");
            list.push_back (item);
        }
        if (list.empty ())
            throw UsageError ("--formats must name one or more formats");
        return list;
    }

    std::pair<int, int> parseBpmRange (const std::string& text)
    {
        const auto dash = text.find ('-');
        if (dash == std::string::npos)
        {
            if (!isNumber (text))
                throw UsageError ("The BPM must be a number such as 96, or a range such as 90-110: " + text);
            const int bpm = std::stoi (text);
            return { bpm, bpm };
        }

        const auto lo = text.substr (0, dash);
        const auto hi = text.substr (dash + 1);
        if (!isNumber (lo) || !isNumber (hi))
            throw UsageError ("The BPM must be a number such as 96, or a range such as 90-110: " + text);
        return { std::stoi (lo), std::stoi (hi) };
    }

    std::string parseFormCode (const std::string& text, const DataSet& data)
    {
        const auto code = lower (text);
        if (code == "vc")
            return "Verse-Chorus";
        if (code == "vcb")
            return "Verse-Chorus-Bridge";
        if (code == "loop")
            return "Loop";
        for (const auto& form : data.forms)
            if (lower (form.name) == code)
                return form.name;
        throw UsageError ("Unknown form: " + text + ". Use vc, vcb or loop.");
    }

    GenerationSettings SettingsArgs::toSettings (const DataSet& data) const
    {
        GenerationSettings settings;

        if (lower (key) != "any")
        {
            const auto note = tryParseNoteName (key);
            if (!note || note->midiNote)
                throw UsageError ("Unknown key: " + key + ". Use a note name such as G, F# or Bb, or \"any\".");
            settings.keyRoot = note->pitchClass;
        }

        if (!scale.empty ())
        {
            const auto names = data.scales.allNames ();
            const auto match = std::find_if (names.begin (), names.end (), [this] (const std::string& name)
            {
                return lower (name) == lower (scale);
            });
            if (match == names.end ())
                throw UsageError ("Unknown scale: " + scale);
            settings.scale = *match;
        }

        std::string moodName = lower (mood);
        if (!moodName.empty ())
            moodName[0] = static_cast<char> (std::toupper (static_cast<unsigned char> (moodName[0])));
        const auto parsedMood = parseMood (moodName);
        if (!parsedMood)
            throw UsageError ("Unknown mood: " + mood + ". Use bright, sad, hopeful or bluesy.");
        settings.mood = *parsedMood;

        std::string difficultyText = lower (difficulty);
        if (!difficultyText.empty ())
            difficultyText[0] = static_cast<char> (std::toupper (static_cast<unsigned char> (difficultyText[0])));
        const auto parsedDifficulty = parseDifficulty (difficultyText);
        if (!parsedDifficulty)
            throw UsageError ("Unknown difficulty: " + difficulty + ". Use beginner, intermediate or advanced.");
        settings.difficulty = *parsedDifficulty;

        const auto [lo, hi] = parseBpmRange (bpm);
        settings.bpmMin     = lo;
        settings.bpmMax     = hi;
        settings.form       = parseFormCode (form, data);
        settings.capo       = capo;
        settings.hook       = hook;
        settings.tables     = lower (tables);
        settings.tuning     = tuning;

        if (time == "4/4")
            settings.timeSig = { 4, 4 };
        else if (time == "6/8")
            settings.timeSig = { 6, 8 };
        else
            throw UsageError ("The time signature must be 4/4 or 6/8: " + time);

        const auto errors = validate (settings, data);
        if (!errors.empty ())
        {
            std::string text = "The settings are not valid:";
            for (const auto& error : errors)
                text += "\n  " + error;
            throw UsageError (text);
        }
        return settings;
    }

    std::filesystem::path defaultLearnedFolder ()
    {
#ifdef _WIN32
        const char* home = std::getenv ("USERPROFILE");
#else
        const char* home = std::getenv ("HOME");
#endif
        return std::filesystem::path (home != nullptr ? home : ".") / "Algo-Rythm" / "learned";
    }

    DataSet loadData (const GlobalArgs& global, const std::string& tables, std::vector<std::string>* warnings)
    {
        const DataSet base   = global.dataFolder.empty () ? bundledData () : loadDataFolder (global.dataFolder);
        const auto    folder = global.learnedFolder.empty () ? defaultLearnedFolder () : std::filesystem::path (global.learnedFolder);
        return dataForTables (base, lower (tables), folder, warnings);
    }

    std::uint64_t clockSeed ()
    {
        const auto now = std::chrono::system_clock::now ().time_since_epoch ();
        return static_cast<std::uint64_t> (std::chrono::duration_cast<std::chrono::microseconds> (now).count ()) % 1000000000ULL;
    }
}

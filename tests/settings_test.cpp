#include "algo/Settings.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

using namespace algo;

namespace
{
    bool hasError (const std::vector<std::string>& errors, const std::string& text)
    {
        return std::any_of (errors.begin (), errors.end (), [&] (const std::string& error)
        {
            return error.find (text) != std::string::npos;
        });
    }

    GenerationSettings withBpm (int lo, int hi)
    {
        GenerationSettings settings;
        settings.bpmMin = lo;
        settings.bpmMax = hi;
        return settings;
    }

    GenerationSettings withCapo (int capo)
    {
        GenerationSettings settings;
        settings.capo = capo;
        return settings;
    }

    GenerationSettings fullSettings ()
    {
        GenerationSettings settings;
        settings.keyRoot    = 7;
        settings.scale      = "Dorian";
        settings.mood       = Mood::Bluesy;
        settings.bpmMin     = 60;
        settings.bpmMax     = 180;
        settings.form       = "Verse-Chorus-Bridge";
        settings.difficulty = Difficulty::Advanced;
        settings.capo       = 3;
        settings.masterSeed = 0xFFFFFFFFFFFFFFFFULL;
        settings.locks.key  = true;
        settings.locks.form = true;
        settings.locks.sections.push_back ({ true, false, false });
        settings.locks.sections.push_back ({ false, true, true });
        return settings;
    }
}

TEST_CASE ("default settings are valid")
{
    REQUIRE (validate (GenerationSettings {}).empty ());
}

TEST_CASE ("default settings have any key and no scale")
{
    const GenerationSettings settings;

    REQUIRE_FALSE (settings.keyRoot.has_value ());
    REQUIRE_FALSE (settings.scale.has_value ());
}

TEST_CASE ("validate checks the BPM range")
{
    REQUIRE (hasError (validate (withBpm (30, 50)), "BPM range"));
    REQUIRE (hasError (validate (withBpm (39, 100)), "BPM range"));
    REQUIRE (hasError (validate (withBpm (100, 221)), "BPM range"));
    REQUIRE (hasError (validate (withBpm (120, 90)), "must not be more"));

    REQUIRE (validate (withBpm (40, 220)).empty ());
    REQUIRE (validate (withBpm (96, 96)).empty ());
}

TEST_CASE ("validate needs an even number in the BPM range")
{
    REQUIRE (hasError (validate (withBpm (91, 91)), "even number"));

    REQUIRE (validate (withBpm (91, 92)).empty ());
    REQUIRE (validate (withBpm (90, 91)).empty ());
}

TEST_CASE ("validate checks the capo")
{
    REQUIRE (validate (withCapo (0)).empty ());
    REQUIRE (validate (withCapo (7)).empty ());
    REQUIRE (hasError (validate (withCapo (-1)), "capo"));
    REQUIRE (hasError (validate (withCapo (8)), "capo"));
}

TEST_CASE ("validate checks the key root")
{
    GenerationSettings settings;

    settings.keyRoot = 0;
    REQUIRE (validate (settings).empty ());
    settings.keyRoot = 11;
    REQUIRE (validate (settings).empty ());

    settings.keyRoot = -1;
    REQUIRE (hasError (validate (settings), "key root"));
    settings.keyRoot = 12;
    REQUIRE (hasError (validate (settings), "key root"));
}

TEST_CASE ("validate checks the scale name with the scale registry")
{
    GenerationSettings settings;

    settings.scale = "Dorian";
    REQUIRE (validate (settings).empty ());

    settings.scale = "Klingon";
    REQUIRE (hasError (validate (settings), "Unknown scale: Klingon"));
}

TEST_CASE ("validate needs a scale with 7 notes")
{
    GenerationSettings settings;

    for (const char* scale : { "Major", "Natural Minor", "Harmonic Minor", "Melodic Minor", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian" })
    {
        settings.scale = scale;
        REQUIRE (validate (settings).empty ());
    }

    for (const char* scale : { "Major Pentatonic", "Blues", "Whole Tone", "Diminished (HW)" })
    {
        settings.scale = scale;
        REQUIRE (hasError (validate (settings), "must have 7 notes"));
    }
}

TEST_CASE ("validate checks the form name")
{
    GenerationSettings settings;

    for (const char* form : { "Verse-Chorus", "Verse-Chorus-Bridge", "Loop" })
    {
        settings.form = form;
        REQUIRE (validate (settings).empty ());
    }

    settings.form = "Fugue";
    REQUIRE (hasError (validate (settings), "Unknown form: Fugue"));
}

TEST_CASE ("validate permits only 4/4 and 6/8 time (T16-06)")
{
    GenerationSettings settings;

    settings.timeSig = { 6, 8 };
    REQUIRE (validate (settings).empty ());

    settings.timeSig = { 3, 4 };
    REQUIRE (hasError (validate (settings), "4/4 or 6/8: 3/4"));
}

TEST_CASE ("validate gives one error for each problem")
{
    GenerationSettings settings;
    settings.bpmMin = 300;
    settings.bpmMax = 200;
    settings.capo   = 9;
    settings.form   = "";

    REQUIRE (validate (settings).size () == 4);
}

TEST_CASE ("moodName and parseMood are the inverse of each other")
{
    for (auto mood : { Mood::Bright, Mood::Sad, Mood::Hopeful, Mood::Bluesy })
        REQUIRE (parseMood (moodName (mood)) == mood);

    REQUIRE_FALSE (parseMood ("Angry").has_value ());
}

TEST_CASE ("settings JSON has the expected text")
{
    const Json j = GenerationSettings {};

    REQUIRE (j.dump () == R"({"key":"any","scale":null,"mood":"Bright","bpmMin":90,"bpmMax":120,"form":"Verse-Chorus","difficulty":"Beginner","capo":0,"timeSig":{"numerator":4,"denominator":4},"masterSeed":0,"locks":{"key":false,"scale":false,"tempo":false,"form":false,"sections":[]},"hook":false,"tables":"rules","tuning":"Standard","rerolls":{"chords":[],"voicings":[],"strums":[],"hook":0}})");
}

TEST_CASE ("settings JSON writes the key root and the scale")
{
    const Json j = fullSettings ();

    REQUIRE (j.at ("key") == 7);
    REQUIRE (j.at ("scale") == "Dorian");
    REQUIRE (j.at ("masterSeed") == 0xFFFFFFFFFFFFFFFFULL);
    REQUIRE (j.at ("locks").at ("sections").at (1).dump () == R"({"chords":false,"voicings":true,"strum":true})");
}

TEST_CASE ("settings go through JSON text and back with no change")
{
    for (const auto& settings : { GenerationSettings {}, fullSettings () })
    {
        const std::string text = Json (settings).dump ();
        const auto        back = Json::parse (text).get<GenerationSettings> ();

        REQUIRE (back == settings);
        REQUIRE (Json (back).dump () == text);
    }
}

TEST_CASE ("reading settings JSON replaces the old values")
{
    GenerationSettings settings = fullSettings ();

    Json (GenerationSettings {}).get_to (settings);

    REQUIRE (settings == GenerationSettings {});
}

TEST_CASE ("settings JSON that is not valid throws")
{
    const Json good = GenerationSettings {};

    auto withField = [&] (const char* key, const Json& value)
    {
        Json j = good;
        j[key] = value;
        return j;
    };

    REQUIRE_THROWS_AS (withField ("key", "G").get<GenerationSettings> (), std::invalid_argument);
    REQUIRE_THROWS_AS (withField ("mood", "Angry").get<GenerationSettings> (), std::invalid_argument);
    REQUIRE_THROWS_AS (withField ("difficulty", "Expert").get<GenerationSettings> (), std::invalid_argument);
    REQUIRE_THROWS_AS (withField ("key", nullptr).get<GenerationSettings> (), Json::exception);

    Json noCapo = good;
    noCapo.erase ("capo");
    REQUIRE_THROWS_AS (noCapo.get<GenerationSettings> (), Json::exception);
}

TEST_CASE ("settings JSON with a bad value parses, and validate then gives the error")
{
    Json j      = GenerationSettings {};
    j["bpmMin"] = 30;
    j["bpmMax"] = 50;

    REQUIRE (hasError (validate (j.get<GenerationSettings> ()), "BPM range"));
}

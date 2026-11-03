#include "algo/Engine.h"
#include "algo/Files.h"

#include "support/Golden.h"
#include "support/SmfReader.h"
#include "support/StarterHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

using namespace algo;

namespace
{
    std::filesystem::path tempFolder (const std::string& name)
    {
        const auto folder = std::filesystem::temp_directory_path () / ("algo-files-test-" + name);
        std::filesystem::remove_all (folder);
        return folder;
    }
}

TEST_CASE ("Save then load gives an equal SongStarter")
{
    const auto folder = tempFolder ("save");
    std::filesystem::create_directories (folder);

    auto starter    = generate (algo::test::settingsFor (12, Difficulty::Advanced, true));
    starter.rating  = 4;
    const auto path = folder / "starter.json";
    saveStarter (starter, path);
    REQUIRE (loadStarter (path) == starter);

    const auto text = algo::test::readBytes (path);
    REQUIRE (text.rfind ("{\n  \"schemaVersion\": 1,\n  \"seed\": 12,", 0) == 0);
    std::filesystem::remove_all (folder);
}

TEST_CASE ("loadStarter gives a clear error for a file that is not valid")
{
    const auto folder = tempFolder ("bad");
    std::filesystem::create_directories (folder);
    std::ofstream (folder / "bad.json") << "{ \"schemaVersion\": 1 }";
    REQUIRE_THROWS_AS (loadStarter (folder / "bad.json"), std::runtime_error);
    REQUIRE_THROWS_AS (loadStarter (folder / "missing.json"), std::runtime_error);
    std::filesystem::remove_all (folder);
}

TEST_CASE ("The default file name has the key, the scale, the BPM and the seed")
{
    SongStarter starter;
    starter.key  = { 7, "Major" };
    starter.bpm  = 96;
    starter.seed = 81723;
    REQUIRE (defaultFileName (starter) == "G-Major-96bpm-81723");

    starter.key = { 6, "Natural Minor" };
    REQUIRE (defaultFileName (starter) == "Fs-NaturalMinor-96bpm-81723");

    starter.key = { 10, "Major" };
    REQUIRE (defaultFileName (starter) == "Bb-Major-96bpm-81723");
}

TEST_CASE ("exportAll with 3 formats writes 3 files with the same base name and makes the folder")
{
    const auto folder  = tempFolder ("export") / "a" / "b";
    const auto starter = generate (GenerationSettings {});
    const auto paths   = exportAll (starter, folder, { "json", "mid", "tab" });

    REQUIRE (paths.size () == 3);
    for (const auto& path : paths)
    {
        REQUIRE (std::filesystem::exists (path));
        REQUIRE (path.stem ().string () == defaultFileName (starter));
    }
    REQUIRE (paths[0].extension () == ".json");
    REQUIRE (paths[1].extension () == ".mid");
    REQUIRE (paths[2].extension () == ".txt");
    REQUIRE (algo::test::readSmfFile (paths[1].string ()).format == 1);

    REQUIRE_THROWS_AS (exportAll (starter, folder, { "json", "wav" }), std::invalid_argument);
    std::filesystem::remove_all (tempFolder ("export"));
}

TEST_CASE ("The golden JSON files load and save to the same text")
{
    for (const auto& [name, settings] : algo::test::goldenSettings ())
    {
        CAPTURE (name);
        const auto starter = generate (settings);
        algo::test::requireGolden ("json/" + name + ".json", starterJsonText (starter));
        REQUIRE (loadStarter (algo::test::goldenDir () / "json" / (name + ".json")) == starter);
    }
}

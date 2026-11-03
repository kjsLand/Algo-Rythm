#include "algo/DataError.h"
#include "algo/Engine.h"
#include "algo/Strum.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

using namespace algo;

namespace
{
    std::filesystem::path tempFolder ()
    {
        const auto folder = std::filesystem::temp_directory_path () / "algo-tables-test";
        std::filesystem::remove_all (folder);
        std::filesystem::create_directories (folder);
        return folder;
    }
}

TEST_CASE ("The rule tables are the default and the tables setting is validated")
{
    REQUIRE (GenerationSettings {}.tables == "rules");
    GenerationSettings settings;
    settings.tables = "other";
    REQUIRE_FALSE (validate (settings).empty ());
}

TEST_CASE ("The learned tables load if they exist, and if not, the rule tables stay with a warning (REQ-15-4)")
{
    const auto folder = tempFolder ();

    std::vector<std::string> warnings;
    const auto               rules = dataForTables (bundledData (), "learned", folder, &warnings);
    REQUIRE (warnings.size () == 1);
    REQUIRE (rules.transitions.source () == bundledData ().transitions.source ());

    std::string table = R"({ "schemaVersion": 1, "source": "learned test", "songs": 3, "date": "2026-11-03", "tables": [)";
    for (const char* mode : { "major", "minor" })
    {
        table += std::string (R"({ "name": ")") + mode + R"(", "rows": {)";
        for (int from = 1; from <= 7; ++from)
        {
            table += "\"" + std::to_string (from) + "\": { \"" + std::to_string (from % 7 + 1) + "\": 1.0 }";
            table += from < 7 ? "," : "";
        }
        table += std::string ("} }") + (std::string (mode) == "major" ? "," : "");
    }
    table += "] }";
    std::ofstream (folder / "transitions.learned.json") << table;
    std::ofstream (folder / "strums.learned.json") << R"({ "source": "test", "weights": { "Folk Push": 2.5 } })";

    warnings.clear ();
    const auto learned = dataForTables (bundledData (), "learned", folder, &warnings);
    REQUIRE (warnings.empty ());
    REQUIRE (learned.transitions.source () == "learned test");
    REQUIRE (learned.transitions.weight (TransitionMode::Major, 1, 2) == 1.0);
    REQUIRE (StrumLibrary::fromData (learned).weight ("Folk Push") == 2.5);

    // The rule tables ignore the folder.
    REQUIRE (dataForTables (bundledData (), "rules", folder).transitions.source () == bundledData ().transitions.source ());

    std::ofstream (folder / "strums.learned.json") << R"({ "weights": { "Folk Push": -1 } })";
    REQUIRE_THROWS_AS (dataForTables (bundledData (), "learned", folder), DataError);
    std::filesystem::remove_all (folder);
}

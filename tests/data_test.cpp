#include "algo/Data.h"
#include "algo/DataError.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

using namespace algo;
namespace fs = std::filesystem;

namespace
{
    // An empty folder that is removed at the end of the test.
    class TempFolder
    {
    public:
        explicit TempFolder (const std::string& name)
            : m_path (fs::temp_directory_path () / ("algo_data_test_" + name))
        {
            fs::remove_all (m_path);
            fs::create_directories (m_path);
        }

        ~TempFolder ()
        {
            std::error_code error;
            fs::remove_all (m_path, error);
        }

        const fs::path& path () const
        {
            return m_path;
        }

        void write (const std::string& file, const std::string& text) const
        {
            std::ofstream (m_path / file) << text;
        }

    private:
        fs::path m_path;
    };

    // Goes back to the original working folder at the end of the test.
    class WorkingFolder
    {
    public:
        explicit WorkingFolder (const fs::path& path)
            : m_original (fs::current_path ())
        {
            fs::current_path (path);
        }

        ~WorkingFolder ()
        {
            std::error_code error;
            fs::current_path (m_original, error);
        }

    private:
        fs::path m_original;
    };
}

TEST_CASE ("loadBundledData needs no core/data folder")
{
    const TempFolder folder ("bundled");
    REQUIRE_FALSE (fs::exists (folder.path () / "core" / "data"));

    const WorkingFolder working (folder.path ());
    const auto          data = loadBundledData ();

    REQUIRE (data.scales.all ().size () == 15);
    REQUIRE (data.chords.all ().size () == 21);
    REQUIRE (data.progressions.all ().size () >= 20);
    REQUIRE (data.strums.size () >= 12);
    REQUIRE (data.shapes.size () >= 40);
}

TEST_CASE ("loadDataFolder uses the bundled file if a file is not in the folder")
{
    const TempFolder folder ("override");
    folder.write ("strums.json", R"({ "schemaVersion": 1, "strums": [
        { "name": "Only", "steps": 2, "slots": "D-", "accents": [0.6, 0.0], "difficulty": "Beginner", "tags": [] } ] })");

    const auto data    = loadDataFolder (folder.path ());
    const auto bundled = loadBundledData ();

    REQUIRE (data.strums.size () == 1);
    REQUIRE (data.strums[0].name == "Only");
    REQUIRE (data.chords.allNames () == bundled.chords.allNames ());
    REQUIRE (data.shapes.size () == bundled.shapes.size ());
}

TEST_CASE ("loadDataFolder with an empty folder gives the bundled data")
{
    const TempFolder folder ("empty");

    REQUIRE (loadDataFolder (folder.path ()).scales.allNames () == loadBundledData ().scales.allNames ());
}

TEST_CASE ("loadDataFolder stops with the file name if a file is not valid")
{
    const TempFolder folder ("invalid");
    folder.write ("chords.json", "{ not json");

    try
    {
        loadDataFolder (folder.path ());
        FAIL ("No error");
    }
    catch (const DataError& e)
    {
        REQUIRE (e.file == "chords.json");
    }
}

TEST_CASE ("loadDataFolder checks a bundled file against a file from the folder")
{
    // The bundled progressions and shapes use chord types that this file does not have.
    const TempFolder folder ("reference");
    folder.write ("chords.json", R"({ "schemaVersion": 1, "chords": [
        { "name": "Major", "symbol": "", "intervals": [4, 7], "optional": [], "tags": [] } ] })");

    try
    {
        loadDataFolder (folder.path ());
        FAIL ("No error");
    }
    catch (const DataError& e)
    {
        REQUIRE (e.file == "progressions.json");
        REQUIRE (e.entry == "Axis");
    }
}

TEST_CASE ("loadDataFolder stops if the folder does not exist")
{
    REQUIRE_THROWS_AS (loadDataFolder (fs::temp_directory_path () / "algo_data_test_no_such_folder"), DataError);
}

#pragma once

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace algo::test
{
    /** The folder tests/golden. CMake gives the path. */
    inline std::filesystem::path goldenDir ()
    {
        return std::filesystem::path (ALGO_GOLDEN_DIR);
    }

    inline std::string readBytes (const std::filesystem::path& path)
    {
        std::ifstream stream (path, std::ios::binary);
        return std::string (std::istreambuf_iterator<char> (stream), {});
    }

    /** Compares the text to a stored golden file byte for byte. If ALGO_UPDATE_GOLDEN is set, writes the file. */
    inline void requireGolden (const std::filesystem::path& relative, const std::string& actual)
    {
        const auto path = goldenDir () / relative;
        if (std::getenv ("ALGO_UPDATE_GOLDEN") != nullptr)
        {
            std::filesystem::create_directories (path.parent_path ());
            std::ofstream (path, std::ios::binary) << actual;
        }

        INFO ("Golden file: " << path.string () << ". Set ALGO_UPDATE_GOLDEN=1 to write it again.");
        REQUIRE (std::filesystem::exists (path));
        REQUIRE (readBytes (path) == actual);
    }
}

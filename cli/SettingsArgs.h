#pragma once

#include "algo/Data.h"
#include "algo/Settings.h"

#include <CLI/CLI.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace algo::cli
{
    // A usage error gives exit code 2 (REQ-10-3).
    class UsageError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    constexpr int kExitOk      = 0;
    constexpr int kExitFailure = 1;
    constexpr int kExitUsage   = 2;

    // The options of the global command line.
    struct GlobalArgs
    {
        std::string dataFolder;    // Empty = the bundled data
        std::string learnedFolder; // Empty = ~/Algo-Rythm/learned
    };

    // The settings options of generate and batch.
    struct SettingsArgs
    {
        std::string key { "any" };
        std::string scale;
        std::string mood { "bright" };
        std::string bpm { "90-120" };
        std::string form { "vc" };
        std::string difficulty { "beginner" };
        int         capo { 0 };
        std::string time { "4/4" };
        std::string tuning { "Standard" };
        std::string tables { "rules" };
        bool        hook { false };
        bool        fingerpicking { false };
        std::string out { "." };
        std::string formats { "json,mid,tab" };

        void addTo (CLI::App& command);

        // Throws UsageError if a value is not valid.
        GenerationSettings       toSettings (const DataSet& data) const;
        std::vector<std::string> formatList () const;
    };

    // Accepts "90-110" and "96". Throws UsageError for other text.
    std::pair<int, int> parseBpmRange (const std::string& text);

    // Accepts the codes "vc", "vcb" and "loop", and the full form names. Throws UsageError for other text.
    std::string parseFormCode (const std::string& text, const DataSet& data);

    // The data of the --data folder, with the tables of the settings.
    DataSet loadData (const GlobalArgs& global, const std::string& tables, std::vector<std::string>* warnings);

    std::filesystem::path defaultLearnedFolder ();

    // A seed from the system clock.
    std::uint64_t clockSeed ();
}

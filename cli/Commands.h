#pragma once

#include "SettingsArgs.h"

#include <CLI/CLI.hpp>

#include <functional>
#include <iosfwd>

namespace algo::cli
{
    // Runs a command after a parse. Returns the exit code.
    using CommandRun = std::function<int ()>;

    // Each function adds one subcommand and returns the function that runs it.
    CommandRun addGenerateCommand (CLI::App& app, const GlobalArgs& global);
    CommandRun addRenderCommand (CLI::App& app, const GlobalArgs& global);
    CommandRun addBatchCommand (CLI::App& app, const GlobalArgs& global);
    CommandRun addCheckCommand (CLI::App& app, const GlobalArgs& global);

    // Writes the warnings to standard error.
    void printWarnings (const std::vector<std::string>& warnings);
}

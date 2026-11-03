// algo-cli: generates song starters with no user interface (SPEC-10).
// It links only to algo::core and CLI11 (REQ-10-1).
#include "Commands.h"
#include "SettingsArgs.h"

#include "algo/DataError.h"
#include "algo/GenerationError.h"
#include "algo/Version.h"

#include <CLI/CLI.hpp>

#include <iostream>
#include <map>
#include <string>

int main (int argc, char** argv)
{
    using namespace algo::cli;

    CLI::App app { "algo-cli: generate guitar song starters as JSON, MIDI, ASCII tab and MusicXML", "algo-cli" };
    app.require_subcommand (1);
    app.set_version_flag ("--version", algo::version ());

    GlobalArgs global;
    app.add_option ("--data", global.dataFolder, "A folder with data files. A file that is not in the folder comes from the bundled data.");
    app.add_option ("--learned", global.learnedFolder, "The folder with the learned tables (default: ~/Algo-Rythm/learned)");

    const std::map<std::string, CommandRun> commands {
        { "generate", addGenerateCommand (app, global) },
        { "render", addRenderCommand (app, global) },
        { "batch", addBatchCommand (app, global) },
        { "check", addCheckCommand (app, global) },
    };

    try
    {
        app.parse (argc, argv);
    }
    catch (const CLI::CallForHelp& e)
    {
        return app.exit (e);
    }
    catch (const CLI::CallForAllHelp& e)
    {
        return app.exit (e);
    }
    catch (const CLI::CallForVersion& e)
    {
        return app.exit (e);
    }
    catch (const CLI::ParseError& e)
    {
        app.exit (e);
        return kExitUsage;
    }

    try
    {
        for (const auto& [name, run] : commands)
            if (app.got_subcommand (name))
                return run ();
    }
    catch (const UsageError& error)
    {
        std::cerr << "Error: " << error.what () << '\n';
        return kExitUsage;
    }
    catch (const algo::DataError& error)
    {
        std::cerr << "Data error: " << error.what () << '\n';
        return kExitUsage;
    }
    catch (const algo::GenerationError& error)
    {
        std::cerr << "Generation error: " << error.what () << '\n';
        return kExitFailure;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what () << '\n';
        return kExitFailure;
    }
    return kExitUsage;
}

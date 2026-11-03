#include "Commands.h"

#include "algo/Files.h"
#include "algo/Validate.h"

#include <iostream>
#include <memory>

namespace algo::cli
{
    CommandRun addCheckCommand (CLI::App& app, const GlobalArgs& global)
    {
        auto* command = app.add_subcommand ("check", "Check song starter JSON files against the voicing, chord, key, strum and hook rules");

        auto files = std::make_shared<std::vector<std::string>> ();
        command->add_option ("files", *files, "The song starter JSON files")->required ();
        command->footer ("Exit code 0: all files pass. Exit code 1: one or more failures.\nExample:\n  algo-cli check songs/*.json");

        return [files, &global] ()
        {
            const auto data     = loadData (global, "rules", nullptr);
            int        failures = 0;
            for (const auto& file : *files)
            {
                try
                {
                    const auto starter = loadStarter (file);
                    const auto found   = validateStarter (starter, data);
                    for (const auto& failure : found)
                        std::cout << file << ": " << failureText (failure) << '\n';
                    failures += static_cast<int> (found.size ());
                    if (found.empty ())
                        std::cout << file << ": OK\n";
                }
                catch (const std::exception& error)
                {
                    std::cout << file << ": load: " << error.what () << '\n';
                    ++failures;
                }
            }
            if (failures > 0)
                std::cerr << failures << " failure" << (failures == 1 ? "" : "s") << '\n';
            return failures > 0 ? kExitFailure : kExitOk;
        };
    }
}

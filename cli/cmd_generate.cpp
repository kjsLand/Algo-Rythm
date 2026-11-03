#include "Commands.h"

#include "algo/Engine.h"
#include "algo/Files.h"

#include <iostream>
#include <memory>
#include <optional>

namespace algo::cli
{
    void printWarnings (const std::vector<std::string>& warnings)
    {
        for (const auto& warning : warnings)
            std::cerr << "Warning: " << warning << '\n';
    }

    CommandRun addGenerateCommand (CLI::App& app, const GlobalArgs& global)
    {
        auto* command = app.add_subcommand ("generate", "Generate one song starter and write it in each format");

        auto args = std::make_shared<SettingsArgs> ();
        auto seed = std::make_shared<std::optional<std::uint64_t>> ();
        args->addTo (*command);
        command->add_option ("--seed", *seed, "The seed. If you do not give a seed, the system clock gives one.");
        command->footer ("Example:\n  algo-cli generate --key G --mood bright --bpm 90-110 --form vcb --difficulty beginner --seed 81723 --out songs");

        return [args, seed, &global] ()
        {
            std::vector<std::string> warnings;
            const auto               data = loadData (global, args->tables, &warnings);
            printWarnings (warnings);

            auto       settings = args->toSettings (data);
            const auto formats  = args->formatList ();
            settings.masterSeed = seed->has_value () ? **seed : clockSeed ();
            std::cout << "Seed: " << settings.masterSeed << '\n';

            const auto starter = generate (settings, data);
            for (const auto& path : exportAll (starter, args->out, formats, data))
                std::cout << path.string () << '\n';
            return kExitOk;
        };
    }
}

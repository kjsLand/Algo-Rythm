#include "Commands.h"

#include "algo/Files.h"

#include <filesystem>
#include <iostream>
#include <memory>

namespace algo::cli
{
    CommandRun addRenderCommand (CLI::App& app, const GlobalArgs& global)
    {
        auto* command = app.add_subcommand ("render", "Render a song starter JSON file to a MIDI file or an ASCII tab file");

        auto input = std::make_shared<std::string> ();
        auto out   = std::make_shared<std::string> ();
        command->add_option ("input", *input, "The song starter JSON file")->required ();
        command->add_option ("--out", *out, "The output file. The extension chooses the format: .mid, .txt or .musicxml.")->required ();
        command->footer ("Example:\n  algo-cli render starter.json --out starter.mid");

        return [input, out, &global] ()
        {
            const std::filesystem::path path (*out);
            const auto                  extension = path.extension ().string ();
            if (extension != ".mid" && extension != ".txt" && extension != ".musicxml")
                throw UsageError ("Unknown output extension: '" + extension + "'. Use .mid, .txt or .musicxml.");

            const auto data    = loadData (global, "rules", nullptr);
            const auto starter = loadStarter (*input);
            if (path.has_parent_path ())
                std::filesystem::create_directories (path.parent_path ());
            exportFile (starter, path, data);
            std::cout << path.string () << '\n';
            return kExitOk;
        };
    }
}

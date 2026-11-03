#include "Commands.h"

#include "algo/Engine.h"
#include "algo/Files.h"
#include "algo/Nashville.h"
#include "algo/Pitch.h"
#include "algo/VoicingCache.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

namespace algo::cli
{
    namespace
    {
        constexpr int kProgressStep = 50;

        std::string csvField (const std::string& text)
        {
            if (text.find_first_of (",\"") == std::string::npos)
                return text;
            std::string quoted = "\"";
            for (char c : text)
                quoted += c == '"' ? std::string ("\"\"") : std::string (1, c);
            return quoted + "\"";
        }

        // The numerals of the first section with the role "verse", or of the first section.
        std::string verseProgression (const SongStarter& starter)
        {
            const Section* verse = starter.sections.empty () ? nullptr : &starter.sections.front ();
            for (const auto& section : starter.sections)
                if (section.role == "verse")
                {
                    verse = &section;
                    break;
                }
            if (verse == nullptr)
                return {};

            std::string text;
            std::size_t count = 0;
            for (const auto& bar : verse->bars)
                for (const auto& chord : bar.chords)
                {
                    if (count++ >= 4)
                        return text;
                    text += (text.empty () ? "" : " ") + numeralText (chord);
                }
            return text;
        }
    }

    CommandRun addBatchCommand (CLI::App& app, const GlobalArgs& global)
    {
        auto* command = app.add_subcommand ("batch", "Generate many song starters and an index.csv file");

        auto args      = std::make_shared<SettingsArgs> ();
        auto count     = std::make_shared<int> (20);
        auto startSeed = std::make_shared<std::uint64_t> (1);
        args->addTo (*command);
        command->add_option ("--count", *count, "The number of song starters")->capture_default_str ();
        command->add_option ("--start-seed", *startSeed, "The seed of the first song starter. The seeds go up by 1.")->capture_default_str ();
        command->footer ("Example:\n  algo-cli batch --count 200 --start-seed 1000 --difficulty beginner --out batch");

        return [args, count, startSeed, &global] ()
        {
            if (*count < 1)
                throw UsageError ("--count must be 1 or more");

            std::vector<std::string> warnings;
            const auto               data = loadData (global, args->tables, &warnings);
            printWarnings (warnings);

            auto       settings = args->toSettings (data);
            const auto formats  = args->formatList ();

            std::filesystem::create_directories (args->out);
            const auto    indexPath = std::filesystem::path (args->out) / "index.csv";
            std::ofstream index (indexPath, std::ios::binary);
            if (!index)
                throw std::runtime_error ("The file cannot be written: " + indexPath.string ());
            index << "seed,key,scale,bpm,form,verse\n";

            VoicingCache cache;
            for (int i = 0; i < *count; ++i)
            {
                settings.masterSeed = *startSeed + static_cast<std::uint64_t> (i);
                const auto starter  = generate (settings, data, nullptr, &cache);
                exportAll (starter, args->out, formats, data);

                index << starter.seed << ',' << csvField (pitchClassName (starter.key.root, starter.key)) << ',' << csvField (starter.key.scale) << ','
                      << starter.bpm << ',' << csvField (starter.form) << ',' << csvField (verseProgression (starter)) << '\n';

                if ((i + 1) % kProgressStep == 0 || i + 1 == *count)
                    std::cout << "Generated " << (i + 1) << " of " << *count << '\n';
            }
            std::cout << indexPath.string () << '\n';
            return kExitOk;
        };
    }
}

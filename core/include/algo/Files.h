#pragma once

#include "Data.h"
#include "Types.h"

#include <filesystem>
#include <string>
#include <vector>

namespace algo
{
    // Writes the song starter as JSON with 2-space indentation and the field order of to_json.
    // Throws std::runtime_error if the file cannot be written.
    void saveStarter (const SongStarter& starter, const std::filesystem::path& path);

    std::string starterJsonText (const SongStarter& starter);

    // Throws std::runtime_error if the file cannot be read or is not a valid song starter.
    SongStarter loadStarter (const std::filesystem::path& path);

    SongStarter parseStarter (const std::string& text);

    // Returns text such as "G-Major-96bpm-81723". A space in the scale name becomes nothing, and '#' becomes "s".
    std::string defaultFileName (const SongStarter& starter);

    // The file extension of a format: "json" = ".json", "mid" = ".mid", "tab" = ".txt", "musicxml" = ".musicxml".
    // Throws std::invalid_argument if the format is not known.
    std::string formatExtension (const std::string& format);

    // Writes one file for each format, with the same base name. Makes the folder if it does not exist.
    // Returns the paths of the files, in the order of the formats.
    std::vector<std::filesystem::path> exportAll (const SongStarter& starter, const std::filesystem::path& folder, const std::vector<std::string>& formats, const DataSet& data = bundledData ());

    // Writes one file. The extension of the path chooses the format: ".json", ".mid", ".txt" or ".musicxml".
    // Throws std::invalid_argument if the extension is not known.
    void exportFile (const SongStarter& starter, const std::filesystem::path& path, const DataSet& data = bundledData ());
}

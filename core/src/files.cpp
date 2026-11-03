#include "algo/Files.h"

#include "algo/Json.h"
#include "algo/MusicXml.h"
#include "algo/Pitch.h"
#include "algo/Render.h"
#include "algo/Smf.h"
#include "algo/Tab.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace algo
{
    namespace
    {
        void writeText (const std::filesystem::path& path, const std::string& text)
        {
            std::ofstream stream (path, std::ios::binary);
            if (!stream)
                throw std::runtime_error ("The file cannot be written: " + path.string ());
            stream << text;
            if (!stream)
                throw std::runtime_error ("The file cannot be written: " + path.string ());
        }
    }

    std::string starterJsonText (const SongStarter& starter)
    {
        return Json (starter).dump (2) + "\n";
    }

    void saveStarter (const SongStarter& starter, const std::filesystem::path& path)
    {
        writeText (path, starterJsonText (starter));
    }

    SongStarter parseStarter (const std::string& text)
    {
        try
        {
            return Json::parse (text).get<SongStarter> ();
        }
        catch (const std::exception& error)
        {
            throw std::runtime_error (std::string ("The song starter is not valid: ") + error.what ());
        }
    }

    SongStarter loadStarter (const std::filesystem::path& path)
    {
        std::ifstream stream (path, std::ios::binary);
        if (!stream)
            throw std::runtime_error ("The file cannot be read: " + path.string ());

        std::ostringstream text;
        text << stream.rdbuf ();
        try
        {
            return parseStarter (text.str ());
        }
        catch (const std::exception& error)
        {
            throw std::runtime_error (path.filename ().string () + ": " + error.what ());
        }
    }

    std::string defaultFileName (const SongStarter& starter)
    {
        std::string key;
        for (char c : pitchClassName (starter.key.root, starter.key))
            key += c == '#' ? std::string ("s") : std::string (1, c);

        std::string scale;
        for (char c : starter.key.scale)
            if (c != ' ' && c != '(' && c != ')')
                scale += c;

        return key + "-" + scale + "-" + std::to_string (starter.bpm) + "bpm-" + std::to_string (starter.seed);
    }

    std::string formatExtension (const std::string& format)
    {
        if (format == "json")
            return ".json";
        if (format == "mid")
            return ".mid";
        if (format == "tab")
            return ".txt";
        if (format == "musicxml")
            return ".musicxml";
        throw std::invalid_argument ("Unknown format: " + format + ". Use json, mid, tab or musicxml.");
    }

    void exportFile (const SongStarter& starter, const std::filesystem::path& path, const DataSet& data)
    {
        const auto extension = path.extension ().string ();
        if (extension == ".json")
            saveStarter (starter, path);
        else if (extension == ".mid")
            writeSmfFile (render (starter, RenderOptions {}, data), path);
        else if (extension == ".txt")
            writeText (path, writeTab (starter, data));
        else if (extension == ".musicxml")
            writeText (path, writeMusicXml (starter, data));
        else
            throw std::invalid_argument ("Unknown file extension: '" + extension + "'. Use .json, .mid, .txt or .musicxml.");
    }

    std::vector<std::filesystem::path> exportAll (const SongStarter& starter, const std::filesystem::path& folder, const std::vector<std::string>& formats, const DataSet& data)
    {
        for (const auto& format : formats)
            formatExtension (format);

        std::error_code error;
        std::filesystem::create_directories (folder, error);
        if (error)
            throw std::runtime_error ("The folder cannot be made: " + folder.string () + ": " + error.message ());

        const auto                         base = defaultFileName (starter);
        std::vector<std::filesystem::path> paths;
        for (const auto& format : formats)
        {
            const auto path = folder / (base + formatExtension (format));
            exportFile (starter, path, data);
            paths.push_back (path);
        }
        return paths;
    }
}

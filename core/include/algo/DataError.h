#pragma once

#include <stdexcept>
#include <string>

namespace algo
{
    // A data file is not valid. what() gives the file, the entry and the problem.
    class DataError : public std::runtime_error
    {
    public:
        DataError (const std::string& fileName, const std::string& entryName, const std::string& problem)
            : std::runtime_error (fileName + ": " + (entryName.empty () ? "" : "entry '" + entryName + "': ") + problem),
              file (fileName),
              entry (entryName),
              message (problem)
        {
        }

        std::string file;
        std::string entry; // Empty if the problem is in the file and not in one entry
        std::string message;
    };
}

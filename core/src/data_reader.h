#pragma once

#include "algo/Json.h"

#include <string>
#include <vector>

namespace algo::detail
{
    std::string toLower (std::string text);

    // Reads one entry of a data file. Each get function checks that the field
    // is present and has the correct JSON type. If not, it throws DataError.
    class EntryReader
    {
    public:
        EntryReader (Json item, std::string file, std::string name);

        const std::string& name () const
        {
            return m_name;
        }

        std::string              getString (const char* key) const;
        int                      getInt (const char* key) const;
        double                   getNumber (const char* key) const;
        bool                     getBool (const char* key) const;
        bool                     getBoolOr (const char* key, bool fallback) const;
        Difficulty               getDifficulty (const char* key) const;
        std::vector<int>         getIntList (const char* key) const;
        std::vector<double>      getNumberList (const char* key) const;
        std::vector<std::string> getStringList (const char* key) const;
        const Json&              getList (const char* key) const;

        [[noreturn]] void fail (const std::string& problem) const;

    private:
        const Json& field (const char* key) const;

        Json        m_item;
        std::string m_file;
        std::string m_name;
    };

    // Parses a data file and checks its schemaVersion. Returns the entries in file order.
    // Each entry has a name, and each name is unique in the file. Case does not change a name.
    std::vector<EntryReader> readEntries (const std::string& text, const std::string& file, const char* listName);
}

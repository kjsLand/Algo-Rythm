#include "data_reader.h"

#include "algo/DataError.h"

#include <cctype>
#include <set>
#include <utility>

namespace algo::detail
{
    namespace
    {
        constexpr int kDataSchemaVersion = 1;
    }

    std::string toLower (std::string text)
    {
        for (char& c : text)
            c = static_cast<char> (std::tolower (static_cast<unsigned char> (c)));
        return text;
    }

    EntryReader::EntryReader (Json item, std::string file, std::string name)
        : m_item (std::move (item)), m_file (std::move (file)), m_name (std::move (name))
    {
    }

    void EntryReader::fail (const std::string& problem) const
    {
        throw DataError (m_file, m_name, problem);
    }

    const Json& EntryReader::field (const char* key) const
    {
        if (!m_item.contains (key))
            fail ("the required field '" + std::string (key) + "' is missing");
        return m_item.at (key);
    }

    std::string EntryReader::getString (const char* key) const
    {
        const Json& value = field (key);
        if (!value.is_string ())
            fail ("the field '" + std::string (key) + "' must be a string");
        return value.get<std::string> ();
    }

    int EntryReader::getInt (const char* key) const
    {
        const Json& value = field (key);
        if (!value.is_number_integer ())
            fail ("the field '" + std::string (key) + "' must be an integer");
        return value.get<int> ();
    }

    double EntryReader::getNumber (const char* key) const
    {
        const Json& value = field (key);
        if (!value.is_number ())
            fail ("the field '" + std::string (key) + "' must be a number");
        return value.get<double> ();
    }

    bool EntryReader::getBool (const char* key) const
    {
        const Json& value = field (key);
        if (!value.is_boolean ())
            fail ("the field '" + std::string (key) + "' must be true or false");
        return value.get<bool> ();
    }

    bool EntryReader::getBoolOr (const char* key, bool fallback) const
    {
        return m_item.contains (key) ? getBool (key) : fallback;
    }

    Difficulty EntryReader::getDifficulty (const char* key) const
    {
        const auto text       = getString (key);
        const auto difficulty = parseDifficulty (text);
        if (!difficulty)
            fail ("the field '" + std::string (key) + "' has the unknown difficulty '" + text + "'");
        return *difficulty;
    }

    const Json& EntryReader::getList (const char* key) const
    {
        const Json& value = field (key);
        if (!value.is_array ())
            fail ("the field '" + std::string (key) + "' must be a list");
        return value;
    }

    std::vector<int> EntryReader::getIntList (const char* key) const
    {
        std::vector<int> out;
        for (const Json& value : getList (key))
        {
            if (!value.is_number_integer ())
                fail ("the field '" + std::string (key) + "' must contain only integers");
            out.push_back (value.get<int> ());
        }
        return out;
    }

    std::vector<double> EntryReader::getNumberList (const char* key) const
    {
        std::vector<double> out;
        for (const Json& value : getList (key))
        {
            if (!value.is_number ())
                fail ("the field '" + std::string (key) + "' must contain only numbers");
            out.push_back (value.get<double> ());
        }
        return out;
    }

    std::vector<std::string> EntryReader::getStringList (const char* key) const
    {
        std::vector<std::string> out;
        for (const Json& value : getList (key))
        {
            if (!value.is_string ())
                fail ("the field '" + std::string (key) + "' must contain only strings");
            out.push_back (value.get<std::string> ());
        }
        return out;
    }

    std::vector<EntryReader> readEntries (const std::string& text, const std::string& file, const char* listName)
    {
        const Json document = Json::parse (text, nullptr, false);
        if (document.is_discarded ())
            throw DataError (file, "", "the file is not valid JSON");
        if (!document.is_object ())
            throw DataError (file, "", "the top level must be an object");

        if (!document.contains ("schemaVersion"))
            throw DataError (file, "", "the required field 'schemaVersion' is missing");
        const Json& version = document.at ("schemaVersion");
        if (!version.is_number_integer ())
            throw DataError (file, "", "the field 'schemaVersion' must be an integer");
        if (version.get<int> () > kDataSchemaVersion)
            throw DataError (file, "", "schemaVersion " + std::to_string (version.get<int> ()) + " is larger than the supported version " + std::to_string (kDataSchemaVersion));

        if (!document.contains (listName))
            throw DataError (file, "", "the required field '" + std::string (listName) + "' is missing");
        const Json& list = document.at (listName);
        if (!list.is_array ())
            throw DataError (file, "", "the field '" + std::string (listName) + "' must be a list");

        std::vector<EntryReader> entries;
        std::set<std::string>    names;
        for (const Json& item : list)
        {
            // An entry with no usable name is identified by its position.
            const std::string position = "#" + std::to_string (entries.size () + 1);
            if (!item.is_object ())
                throw DataError (file, position, "the entry must be an object");
            if (!item.contains ("name"))
                throw DataError (file, position, "the required field 'name' is missing");
            if (!item.at ("name").is_string ())
                throw DataError (file, position, "the field 'name' must be a string");

            const auto name = item.at ("name").get<std::string> ();
            if (!names.insert (toLower (name)).second)
                throw DataError (file, name, "the name is not unique in the file");

            entries.emplace_back (item, file, name);
        }
        return entries;
    }
}

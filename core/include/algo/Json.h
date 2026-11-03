#pragma once

#include "Types.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace algo
{
    // Keeps the fields in the order that they were written.
    using Json = nlohmann::ordered_json;

    std::string               difficultyName (Difficulty difficulty);
    std::optional<Difficulty> parseDifficulty (const std::string& name);

    // 'D', 'U', '-', 'x' or 'c'. A Pluck has the character of its string: refer to slotChar.
    char                  strokeChar (Stroke stroke);
    std::optional<Stroke> parseStroke (char c);

    // The character of one slot: a stroke character, or for a Pluck '1' to '6' (a guitar string),
    // 'b' (the bass string) or 'a' (the alternate bass string).
    char slotChar (const StrumPattern& pattern, std::size_t slot);

    // Reads a slot character. Returns false if the character is not valid.
    bool parseSlotChar (char c, Stroke& stroke, int& pluck);

    // Sets the slots (and the pluck strings) of a pattern from text such as "D-DU" or "b3a2".
    // Returns the first character that is not valid, or 0.
    char setSlots (StrumPattern& pattern, const std::string& text);

    // The pluck string of a slot, or kPluckNone.
    int pluckAt (const StrumPattern& pattern, std::size_t slot);

    void to_json (Json& j, Difficulty v);
    void from_json (const Json& j, Difficulty& v);
    void to_json (Json& j, Stroke v);
    void from_json (const Json& j, Stroke& v);
    void to_json (Json& j, const Key& v);
    void from_json (const Json& j, Key& v);
    void to_json (Json& j, const TimeSig& v);
    void from_json (const Json& j, TimeSig& v);
    void to_json (Json& j, const ChordSymbol& v);
    void from_json (const Json& j, ChordSymbol& v);
    void to_json (Json& j, const FretPos& v);
    void from_json (const Json& j, FretPos& v);
    void to_json (Json& j, const Voicing& v);
    void from_json (const Json& j, Voicing& v);
    void to_json (Json& j, const StrumPattern& v);
    void from_json (const Json& j, StrumPattern& v);
    void to_json (Json& j, const MelodyNote& v);
    void from_json (const Json& j, MelodyNote& v);
    void to_json (Json& j, const Bar& v);
    void from_json (const Json& j, Bar& v);
    void to_json (Json& j, const Section& v);
    void from_json (const Json& j, Section& v);
    void to_json (Json& j, const RerollCounts& v);
    void from_json (const Json& j, RerollCounts& v);
    void to_json (Json& j, const Tuning& v);
    void from_json (const Json& j, Tuning& v);
    void to_json (Json& j, const SongStarter& v);

    // Throws std::runtime_error if schemaVersion is absent or larger than kSchemaVersion.
    void from_json (const Json& j, SongStarter& v);
}

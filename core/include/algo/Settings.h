#pragma once

#include "Data.h"
#include "Json.h"
#include "Types.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace algo
{
    constexpr int kMinBpm = 40;
    constexpr int kMaxBpm = 220;

    enum class Mood
    {
        Bright,
        Sad,
        Hopeful,
        Bluesy
    };

    std::string         moodName (Mood mood);
    std::optional<Mood> parseMood (const std::string& name);

    struct SectionLocks
    {
        bool chords { false };
        bool voicings { false };
        bool strum { false };
    };

    struct Locks
    {
        bool                      key { false };
        bool                      scale { false };
        bool                      tempo { false };
        bool                      form { false };
        std::vector<SectionLocks> sections;
    };

    // The inputs of one generation. The defaults are valid settings.
    struct GenerationSettings
    {
        std::optional<PitchClass>  keyRoot; // No value = "any"
        std::optional<std::string> scale;   // A name in the scale registry. No value = the mood chooses
        Mood                       mood { Mood::Bright };
        int                        bpmMin { 90 };
        int                        bpmMax { 120 };
        std::string                form { "Verse-Chorus" };
        Difficulty                 difficulty { Difficulty::Beginner };
        int                        capo { 0 };
        TimeSig                    timeSig;
        std::uint64_t              masterSeed { 0 };
        Locks                      locks;
    };

    // Returns one error text for each problem. An empty list means that the settings are valid.
    // Call this before generation starts.
    std::vector<std::string> validate (const GenerationSettings& settings, const DataSet& data = bundledData ());

    inline bool operator== (const SectionLocks& a, const SectionLocks& b)
    {
        return a.chords == b.chords && a.voicings == b.voicings && a.strum == b.strum;
    }

    inline bool operator== (const Locks& a, const Locks& b)
    {
        return a.key == b.key && a.scale == b.scale && a.tempo == b.tempo && a.form == b.form && a.sections == b.sections;
    }

    inline bool operator== (const GenerationSettings& a, const GenerationSettings& b)
    {
        return a.keyRoot == b.keyRoot && a.scale == b.scale && a.mood == b.mood && a.bpmMin == b.bpmMin && a.bpmMax == b.bpmMax && a.form == b.form && a.difficulty == b.difficulty && a.capo == b.capo && a.timeSig == b.timeSig && a.masterSeed == b.masterSeed && a.locks == b.locks;
    }

    void to_json (Json& j, Mood v);
    void from_json (const Json& j, Mood& v);
    void to_json (Json& j, const SectionLocks& v);
    void from_json (const Json& j, SectionLocks& v);
    void to_json (Json& j, const Locks& v);
    void from_json (const Json& j, Locks& v);

    // The key is a root from 0 to 11, or the text "any". A scale with no value is null.
    void to_json (Json& j, const GenerationSettings& v);
    void from_json (const Json& j, GenerationSettings& v);
}

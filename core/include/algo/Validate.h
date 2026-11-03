#pragma once

#include "Data.h"
#include "Types.h"

#include <string>
#include <vector>

namespace algo
{
    // The rule names of the strum, hook and song starter checks. The voicing rules are in Voicing.h.
    inline constexpr const char* kRuleStrumDownbeat   = "strum-downbeat";   // REQ-08-1
    inline constexpr const char* kRuleStrumDirection  = "strum-direction";  // REQ-08-2
    inline constexpr const char* kRuleStrumGrid       = "strum-grid";       // REQ-08-3
    inline constexpr const char* kRuleStrumBeat3      = "strum-beat-3";     // REQ-08-4
    inline constexpr const char* kRuleStrumDensity    = "strum-density";    // REQ-08-5
    inline constexpr const char* kRuleStrumPercussion = "strum-percussion"; // REQ-08-6
    inline constexpr const char* kRuleStrumAccent     = "strum-accent";
    inline constexpr const char* kRuleStrumStrings    = "strum-strings";
    inline constexpr const char* kRuleKey             = "key";
    inline constexpr const char* kRuleTempo           = "tempo";
    inline constexpr const char* kRuleChordTag        = "chord-tag";
    inline constexpr const char* kRuleChordUnknown    = "chord-unknown";
    inline constexpr const char* kRuleVoicingCount    = "voicing-count";
    inline constexpr const char* kRuleHookScale       = "hook-scale";      // REQ-13-1
    inline constexpr const char* kRuleHookChordTone   = "hook-chord-tone"; // REQ-13-2
    inline constexpr const char* kRuleHookRange       = "hook-range";      // REQ-13-3
    inline constexpr const char* kRuleHookEnd         = "hook-end";        // REQ-13-4
    inline constexpr const char* kRuleHookLength      = "hook-length";     // REQ-13-5
    inline constexpr const char* kRuleRating          = "rating";          // REQ-14-4

    // One rule that a song starter breaks. bar is -1 for a rule of the section or of the song.
    struct Failure
    {
        std::string section; // Empty for a rule of the song
        int         bar { -1 };
        std::string rule;
        std::string detail;
    };

    // Text such as "Verse bar 3: chord-tones (Em)".
    std::string failureText (const Failure& failure);

    // The rules of one strum pattern, without REQ-08-5.
    std::vector<std::string> strumViolations (const StrumPattern& pattern, Difficulty difficulty, int bpm, const TimeSig& timeSig = TimeSig {});

    // REQ-08-5: a chorus has 1 or 2 more hits than the verse, if the grid permits it.
    bool chorusDensityOk (const StrumPattern& verse, const StrumPattern& chorus);

    // The hook rules REQ-13-1 to REQ-13-5.
    std::vector<Failure> hookViolations (const SongStarter& starter, const DataSet& data = bundledData ());

    // The checks for key, tempo, chords, voicings, strums and hook. An empty list means that the song starter passes.
    std::vector<Failure> validateStarter (const SongStarter& starter, const DataSet& data = bundledData ());
}

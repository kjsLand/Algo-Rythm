#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace algo
{
    constexpr int kPpq           = 480;
    constexpr int kSchemaVersion = 1;
    constexpr int kNumStrings    = 6;
    constexpr int kMutedString   = -1;

    using PitchClass = int;          // 0-11, 0 = C
    using MidiNote   = int;          // 60 = C4
    using Tick       = std::int64_t; // kPpq ticks = one quarter note

    enum class Difficulty
    {
        Beginner,
        Intermediate,
        Advanced
    };

    // One slot in a strum pattern.
    enum class Stroke
    {
        Down,
        Up,
        Rest,
        Mute,
        Chuck
    };

    struct Key
    {
        PitchClass  root { 0 };
        std::string scale { "Major" }; // A name in the scale registry
    };

    struct TimeSig
    {
        int numerator { 4 };
        int denominator { 4 };
    };

    // A chord as a Nashville number. It has no key until it is resolved.
    struct ChordSymbol
    {
        int                      degree { 1 };           // 1-7
        int                      accidental { 0 };       // -1 = flat, 1 = sharp
        std::string              quality { "diatonic" }; // "diatonic", or a chord symbol
        int                      bassDegree { 0 };       // 0 = no slash bass note
        int                      bassAccidental { 0 };
        std::vector<std::string> tags; // For example "borrowed" or "secondary"
    };

    // String index 0 is the low E string.
    struct FretPos
    {
        int string { 0 };
        int fret { 0 };
    };

    // The frets from the low E string to the high E string.
    struct Voicing
    {
        std::array<int, kNumStrings> frets { kMutedString, kMutedString, kMutedString, kMutedString, kMutedString, kMutedString };
        std::array<int, kNumStrings> fingers { 0, 0, 0, 0, 0, 0 }; // 0 = no finger
    };

    struct StrumPattern
    {
        int                 steps { 8 };
        std::vector<Stroke> slots  = std::vector<Stroke> (8, Stroke::Rest);
        std::vector<double> accent = std::vector<double> (8, 0.0); // 0-1 for each slot
        int                 strings { 6 };                         // The top strings that sound
    };

    struct MelodyNote
    {
        MidiNote pitch { 60 };
        Tick     start { 0 };
        Tick     length { kPpq };
        int      velocity { 100 };
    };

    // voicings[i] is the voicing of chords[i].
    struct Bar
    {
        std::vector<ChordSymbol> chords;
        std::vector<Voicing>     voicings;
    };

    struct Section
    {
        std::string      name { "Verse" };
        std::vector<Bar> bars;
        StrumPattern     strum;
    };

    // The open notes from the low E string to the high E string.
    struct Tuning
    {
        std::string                       name { "Standard" };
        std::array<MidiNote, kNumStrings> strings { 40, 45, 50, 55, 59, 64 };
    };

    struct SongStarter
    {
        int                     schemaVersion { kSchemaVersion };
        std::uint64_t           seed { 0 };
        Key                     key;
        int                     bpm { 120 };
        TimeSig                 timeSig;
        int                     capo { 0 };
        Tuning                  tuning;
        Difficulty              difficulty { Difficulty::Beginner };
        std::string             form { "Verse-Chorus" };
        std::vector<Section>    sections;
        std::vector<MelodyNote> hook; // Empty if the song starter has no hook
    };

    inline bool operator== (const Key& a, const Key& b)
    {
        return a.root == b.root && a.scale == b.scale;
    }

    inline bool operator== (const TimeSig& a, const TimeSig& b)
    {
        return a.numerator == b.numerator && a.denominator == b.denominator;
    }

    inline bool operator== (const ChordSymbol& a, const ChordSymbol& b)
    {
        return a.degree == b.degree && a.accidental == b.accidental && a.quality == b.quality && a.bassDegree == b.bassDegree && a.bassAccidental == b.bassAccidental && a.tags == b.tags;
    }

    inline bool operator== (const FretPos& a, const FretPos& b)
    {
        return a.string == b.string && a.fret == b.fret;
    }

    inline bool operator== (const Voicing& a, const Voicing& b)
    {
        return a.frets == b.frets && a.fingers == b.fingers;
    }

    inline bool operator== (const StrumPattern& a, const StrumPattern& b)
    {
        return a.steps == b.steps && a.slots == b.slots && a.accent == b.accent && a.strings == b.strings;
    }

    inline bool operator== (const MelodyNote& a, const MelodyNote& b)
    {
        return a.pitch == b.pitch && a.start == b.start && a.length == b.length && a.velocity == b.velocity;
    }

    inline bool operator== (const Bar& a, const Bar& b)
    {
        return a.chords == b.chords && a.voicings == b.voicings;
    }

    inline bool operator== (const Section& a, const Section& b)
    {
        return a.name == b.name && a.bars == b.bars && a.strum == b.strum;
    }

    inline bool operator== (const Tuning& a, const Tuning& b)
    {
        return a.name == b.name && a.strings == b.strings;
    }

    inline bool operator== (const SongStarter& a, const SongStarter& b)
    {
        return a.schemaVersion == b.schemaVersion && a.seed == b.seed && a.key == b.key && a.bpm == b.bpm && a.timeSig == b.timeSig && a.capo == b.capo && a.tuning == b.tuning && a.difficulty == b.difficulty && a.form == b.form && a.sections == b.sections && a.hook == b.hook;
    }
}

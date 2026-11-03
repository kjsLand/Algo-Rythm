#pragma once

#include "ChordRegistry.h"
#include "Chords.h"
#include "Data.h"
#include "Fretboard.h"
#include "Types.h"
#include "VoicingCost.h"

#include <optional>
#include <string>
#include <vector>

namespace algo
{
    constexpr int kMaxCandidateFret   = 15;
    constexpr int kMaxFingers         = 4;
    constexpr int kMinStrummedStrings = 4;
    constexpr int kBestSequences      = 5;

    // The names of the rules that violations returns.
    inline constexpr const char* kRuleOneNotePerString = "one-note-per-string"; // REQ-07-1
    inline constexpr const char* kRuleLowestNote       = "lowest-note";         // REQ-07-2
    inline constexpr const char* kRuleChordTones       = "chord-tones";         // REQ-07-3
    inline constexpr const char* kRuleFretSpan         = "fret-span";           // REQ-07-4
    inline constexpr const char* kRuleFingerCount      = "finger-count";        // REQ-07-5
    inline constexpr const char* kRuleStringCount      = "string-count";        // REQ-07-6
    inline constexpr const char* kRuleMutedString      = "muted-string";        // REQ-07-7
    inline constexpr const char* kRuleOtherNote        = "other-note";          // A note is not in the chord

    // A chord with a real root.
    struct ConcreteChord
    {
        PitchClass                root { 0 };
        ChordDefinition           definition;
        std::optional<PitchClass> bass; // The slash bass note
        std::string               name; // For example "Em" or "D/F#"
    };

    // Throws as resolve and chordRootPc do.
    ConcreteChord concreteChord (const ChordSymbol& symbol, const Key& key, const DataSet& data = bundledData ());

    // One finger on the strings from firstString to lastString, with notes at the barre fret.
    struct Barre
    {
        int fret { 0 };
        int firstString { 0 };
        int lastString { 0 };
    };

    // A barre is at the lowest fret and has 2 or more notes. It can cover a string with a higher
    // fret, as in x35553. It cannot cover an open or a muted string, so x02020 has no barre.
    std::optional<Barre> findBarre (const Voicing& voicing);

    // The number of fingers on the frets. A barre counts as one finger.
    int fingerCount (const Voicing& voicing);

    // Returns the voicing with its fingers. Finger 1 goes on a barre. The other fretted notes get the next
    // free fingers in fret order, and the lower string first. Returns no value if a note needs a fifth finger.
    std::optional<Voicing> assignFingers (const Voicing& voicing);

    // Makes the voicings of the shapes that have the chord type. A candidate is not filtered,
    // so a shape can have too few strings for a strum.
    std::vector<Voicing> shapeCandidates (const ConcreteChord& chord, const Fretboard& fretboard, const DataSet& data = bundledData ());

    // Makes the strummed voicings with their frets in a window of 4 frets, from base frets 0 to 12.
    std::vector<Voicing> searchCandidates (const ConcreteChord& chord, const Fretboard& fretboard, Difficulty difficulty);

    // Returns the name of each rule that the voicing breaks. Only a strummed voicing has the rule for
    // the number of strings. If a fret is not on the neck, the result has only kRuleOneNotePerString.
    std::vector<std::string> violations (const Voicing& voicing, const ConcreteChord& chord, Difficulty difficulty, const Fretboard& fretboard = Fretboard {}, bool strummed = true);

    // One voicing for each chord.
    struct VoicingSequence
    {
        std::vector<Voicing> voicings;
        int                  cost { 0 };
    };

    // Returns the k best sequences (Viterbi method), with the lowest total cost first. Equal costs have a
    // fixed order. Throws GenerationError with the chord name if a chord has no candidate, and
    // std::invalid_argument if the lists have different sizes or k is less than 1.
    std::vector<VoicingSequence> chooseSequence (const std::vector<ConcreteChord>& chords, const std::vector<std::vector<Voicing>>& candidates, Difficulty difficulty, const Fretboard& fretboard = Fretboard {}, const CostWeights& weights = CostWeights {}, int k = kBestSequences);
}

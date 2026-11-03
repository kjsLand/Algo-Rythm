#include "algo/Validate.h"

#include "algo/Fretboard.h"
#include "algo/Hook.h"
#include "algo/Json.h"
#include "algo/Nashville.h"
#include "algo/Pitch.h"
#include "algo/Render.h"
#include "algo/Scale.h"
#include "algo/Settings.h"
#include "algo/Strum.h"
#include "algo/Voicing.h"

#include <algorithm>
#include <cstddef>

namespace algo
{
    namespace
    {
        bool hasTag (const ChordSymbol& symbol, const std::string& tag)
        {
            return std::find (symbol.tags.begin (), symbol.tags.end (), tag) != symbol.tags.end ();
        }

        bool containsPc (const std::vector<PitchClass>& pcs, MidiNote note)
        {
            return std::find (pcs.begin (), pcs.end (), pitchClassOf (note)) != pcs.end ();
        }

        void checkStrums (const SongStarter& starter, std::vector<Failure>& failures)
        {
            const StrumPattern* verse = nullptr;
            for (const auto& section : starter.sections)
                if (section.role == "verse" && verse == nullptr)
                    verse = &section.strum;

            for (const auto& section : starter.sections)
            {
                for (const auto& rule : strumViolations (section.strum, starter.difficulty, starter.bpm, starter.timeSig))
                    failures.push_back ({ section.name, -1, rule, strumText (section.strum) });

                if (section.role == "chorus" && verse != nullptr && !chorusDensityOk (*verse, section.strum))
                    failures.push_back ({ section.name, -1, kRuleStrumDensity, std::to_string (hitCount (section.strum)) + " hits, verse " + std::to_string (hitCount (*verse)) });
            }
        }
    }

    std::string failureText (const Failure& failure)
    {
        std::string text = failure.section.empty () ? std::string ("Song") : failure.section;
        if (failure.bar >= 0)
            text += " bar " + std::to_string (failure.bar + 1);
        text += ": " + failure.rule;
        if (!failure.detail.empty ())
            text += " (" + failure.detail + ")";
        return text;
    }

    std::vector<std::string> strumViolations (const StrumPattern& pattern, Difficulty difficulty, int bpm, const TimeSig& timeSig)
    {
        std::vector<std::string> rules;
        const int                steps = static_cast<int> (pattern.slots.size ());

        // A fingerpicking pattern starts with a Pluck of the bass string. A strum starts with a Down stroke.
        const bool picking = !pattern.pluck.empty ();
        if (pattern.slots.empty () || pattern.slots[0] != (picking ? Stroke::Pluck : Stroke::Down))
            rules.push_back (kRuleStrumDownbeat);

        for (int i = 0; i < steps; ++i)
        {
            const auto stroke = pattern.slots[static_cast<std::size_t> (i)];
            if ((stroke == Stroke::Down && i % 2 != 0) || (stroke == Stroke::Up && i % 2 == 0))
            {
                rules.push_back (kRuleStrumDirection);
                break;
            }
        }

        const int grid = picking ? fingerpickingSteps (timeSig) : gridSteps (difficulty, bpm, timeSig);
        if (pattern.steps != steps || pattern.steps != grid)
            rules.push_back (kRuleStrumGrid);

        if (std::any_of (pattern.slots.begin (), pattern.slots.end (), [] (Stroke s)
        {
            return s == Stroke::Pluck;
        }) != picking)
            rules.push_back (kRuleStrumDirection);

        if (difficulty == Difficulty::Beginner && steps > 0 && pattern.slots[static_cast<std::size_t> (steps / 2)] == Stroke::Rest)
            rules.push_back (kRuleStrumBeat3);

        if (difficulty != Difficulty::Advanced && std::any_of (pattern.slots.begin (), pattern.slots.end (), [] (Stroke s)
        {
            return s == Stroke::Mute || s == Stroke::Chuck;
        }))
            rules.push_back (kRuleStrumPercussion);

        if (static_cast<int> (pattern.accent.size ()) != steps || std::any_of (pattern.accent.begin (), pattern.accent.end (), [] (double a)
        {
            return !(a >= 0.0 && a <= 1.0);
        }))
            rules.push_back (kRuleStrumAccent);

        if (pattern.strings < 1 || pattern.strings > kNumStrings || (difficulty == Difficulty::Beginner && pattern.strings != kNumStrings))
            rules.push_back (kRuleStrumStrings);

        return rules;
    }

    bool chorusDensityOk (const StrumPattern& verse, const StrumPattern& chorus)
    {
        // A fingerpicking pattern picks each slot, so its density does not change.
        if (!verse.pluck.empty () || !chorus.pluck.empty ())
            return true;

        const int extra = hitCount (chorus) - hitCount (verse);
        if (extra == 1 || extra == 2)
            return true;

        // The grid has no space for more hits.
        return hitCount (verse) + 1 > chorus.steps && extra >= 0;
    }

    std::vector<Failure> hookViolations (const SongStarter& starter, const DataSet& data)
    {
        std::vector<Failure> failures;
        if (starter.hook.empty ())
            return failures;

        const auto starts = sectionStarts (starter);
        const Tick bar    = barTicks (starter.timeSig);
        const auto scale  = scalePitchClasses (starter.key, data);

        const auto sectionOf = [&starts] (Tick tick)
        {
            for (std::size_t i = 0; i + 1 < starts.size (); ++i)
                if (tick >= starts[i] && tick < starts[i + 1])
                    return static_cast<int> (i);
            return -1;
        };

        const auto [lowIt, highIt] = std::minmax_element (starter.hook.begin (), starter.hook.end (), [] (const MelodyNote& a, const MelodyNote& b)
        {
            return a.pitch < b.pitch;
        });
        if (highIt->pitch - lowIt->pitch > kHookRange)
            failures.push_back ({ "Hook", -1, kRuleHookRange, std::to_string (highIt->pitch - lowIt->pitch) + " semitones" });

        for (std::size_t n = 0; n < starter.hook.size (); ++n)
        {
            const auto& note  = starter.hook[n];
            const int   index = sectionOf (note.start);
            if (index < 0)
            {
                failures.push_back ({ "Hook", -1, kRuleHookScale, "a note is after the end of the song" });
                continue;
            }

            const auto& section = starter.sections[static_cast<std::size_t> (index)];
            const Tick  inSect  = note.start - starts[static_cast<std::size_t> (index)];
            const int   barNum  = static_cast<int> (inSect / bar);
            const auto  name    = midiNoteName (note.pitch, starter.key);

            if (!containsPc (scale, note.pitch))
                failures.push_back ({ section.name, barNum, kRuleHookScale, name });

            if (isStrongBeat (inSect % bar, starter.timeSig))
            {
                const auto tones = hookChordTones (section.bars, inSect, starter.timeSig, starter.key, data);
                if (!tones.empty () && !containsPc (tones, note.pitch))
                    failures.push_back ({ section.name, barNum, kRuleHookChordTone, name });
            }

            if (note.length > kMaxHookNote || note.length <= 0)
                failures.push_back ({ section.name, barNum, kRuleHookLength, std::to_string (note.length) + " ticks" });

            if (n + 1 == starter.hook.size ())
            {
                const auto tones = hookEndTones (section.bars, inSect, starter.timeSig, starter.key, data);
                if (!tones.empty () && !containsPc (tones, note.pitch))
                    failures.push_back ({ section.name, barNum, kRuleHookEnd, name });
            }
        }
        return failures;
    }

    std::vector<Failure> validateStarter (const SongStarter& starter, const DataSet& data)
    {
        std::vector<Failure> failures;

        if (starter.key.root < 0 || starter.key.root > 11 || !data.scales.contains (starter.key.scale))
        {
            failures.push_back ({ "", -1, kRuleKey, std::to_string (starter.key.root) + " " + starter.key.scale });
            return failures;
        }
        if (starter.bpm < kMinBpm || starter.bpm > kMaxBpm)
            failures.push_back ({ "", -1, kRuleTempo, std::to_string (starter.bpm) });
        if (starter.rating && (*starter.rating < kMinRating || *starter.rating > kMaxRating))
            failures.push_back ({ "", -1, kRuleRating, std::to_string (*starter.rating) });

        Fretboard fretboard;
        try
        {
            fretboard = Fretboard (starter.tuning, starter.capo);
        }
        catch (const std::exception& error)
        {
            failures.push_back ({ "", -1, kRuleKey, error.what () });
            return failures;
        }

        for (const auto& section : starter.sections)
        {
            for (std::size_t b = 0; b < section.bars.size (); ++b)
            {
                const auto& bar    = section.bars[b];
                const int   barNum = static_cast<int> (b);

                if (bar.voicings.size () != bar.chords.size ())
                    failures.push_back ({ section.name, barNum, kRuleVoicingCount, std::to_string (bar.voicings.size ()) + " voicings for " + std::to_string (bar.chords.size ()) + " chords" });

                for (std::size_t c = 0; c < bar.chords.size (); ++c)
                {
                    const auto& symbol = bar.chords[c];
                    try
                    {
                        const auto name = chordName (symbol, starter.key, data);
                        if (!isDiatonic (symbol, starter.key, data) && !hasTag (symbol, "borrowed") && !hasTag (symbol, "secondary"))
                            failures.push_back ({ section.name, barNum, kRuleChordTag, name });

                        if (c < bar.voicings.size ())
                        {
                            const auto chord = concreteChord (symbol, starter.key, data);
                            for (const auto& rule : violations (bar.voicings[c], chord, starter.difficulty, fretboard, true))
                                failures.push_back ({ section.name, barNum, rule, name });
                        }
                    }
                    catch (const std::exception& error)
                    {
                        failures.push_back ({ section.name, barNum, kRuleChordUnknown, error.what () });
                    }
                }
            }
        }

        checkStrums (starter, failures);

        const auto hook = hookViolations (starter, data);
        failures.insert (failures.end (), hook.begin (), hook.end ());
        return failures;
    }
}

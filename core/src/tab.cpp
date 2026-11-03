#include "algo/Tab.h"

#include "algo/Hook.h"
#include "algo/Nashville.h"
#include "algo/Pitch.h"
#include "algo/Render.h"
#include "algo/Strum.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>

namespace algo
{
    namespace
    {
        using Lines = std::array<std::string, kNumStrings>; // Index 0 is the high E string

        std::array<std::string, kNumStrings> stringLabels (const Tuning& tuning)
        {
            std::array<std::string, kNumStrings> labels;
            if (tuning == Tuning {})
                return { "e", "B", "G", "D", "A", "E" };

            std::size_t width = 1;
            for (int s = 0; s < kNumStrings; ++s)
            {
                labels[static_cast<std::size_t> (kNumStrings - 1 - s)] = pitchClassName (pitchClassOf (tuning.strings[static_cast<std::size_t> (s)]), false);
                width                                                  = std::max (width, labels[static_cast<std::size_t> (kNumStrings - 1 - s)].size ());
            }
            for (auto& label : labels)
                label.resize (width, ' ');
            return labels;
        }

        std::string pad (const std::string& text, std::size_t width)
        {
            return std::string (width > text.size () ? width - text.size () : 0, '-') + text;
        }

        // The cells of one bar. lines[0] is the high E string.
        Lines barCells (const Bar& bar, const StrumPattern& strum)
        {
            std::vector<std::array<std::string, kNumStrings>> columns;
            std::size_t                                       width = 1;
            const int                                         steps = std::max (1, strum.steps);

            for (int slot = 0; slot < steps && slot < static_cast<int> (strum.slots.size ()); ++slot)
            {
                std::array<std::string, kNumStrings> column;
                const auto                           stroke = strum.slots[static_cast<std::size_t> (slot)];
                if (!bar.voicings.empty () && stroke != Stroke::Rest && stroke != Stroke::Chuck)
                {
                    const auto  chord   = std::min (bar.voicings.size () - 1, static_cast<std::size_t> (slot) * bar.voicings.size () / static_cast<std::size_t> (steps));
                    const auto& voicing = bar.voicings[chord];
                    const int   first   = kNumStrings - std::clamp (strum.strings, 1, kNumStrings);

                    for (int s = first; s < kNumStrings; ++s)
                    {
                        const int fret = voicing.frets[static_cast<std::size_t> (s)];
                        if (fret == kMutedString)
                            continue;
                        auto& cell = column[static_cast<std::size_t> (kNumStrings - 1 - s)];
                        cell       = stroke == Stroke::Mute ? "x" : std::to_string (fret);
                        width      = std::max (width, cell.size ());
                    }
                }
                columns.push_back (column);
            }

            Lines lines;
            for (auto& line : lines)
                line = "-";
            for (const auto& column : columns)
                for (std::size_t line = 0; line < kNumStrings; ++line)
                    lines[line] += pad (column[line], width) + "-";
            return lines;
        }

        void writeSystem (std::ostringstream& out, const std::vector<Lines>& bars, const Tuning& tuning)
        {
            const auto labels = stringLabels (tuning);
            for (std::size_t first = 0; first < bars.size (); first += kTabBarsPerLine)
            {
                const auto last = std::min (bars.size (), first + kTabBarsPerLine);
                for (std::size_t line = 0; line < kNumStrings; ++line)
                {
                    out << labels[line] << '|';
                    for (std::size_t b = first; b < last; ++b)
                        out << bars[b][line] << '|';
                    out << '\n';
                }
                out << '\n';
            }
        }

        std::string keyText (const Key& key)
        {
            return pitchClassName (key.root, key) + " " + key.scale;
        }

        // The string and fret of a hook note. The fret stays near the fret of the note before it.
        std::pair<int, int> hookPosition (MidiNote note, const Tuning& tuning, int capo, int& anchor)
        {
            int bestString = -1;
            int bestFret   = 0;
            for (int s = kNumStrings - 1; s >= 0; --s)
            {
                const int fret = note - tuning.strings[static_cast<std::size_t> (s)] - capo;
                if (fret < 0 || fret > 15)
                    continue;
                if (bestString < 0 || std::abs (fret - anchor) < std::abs (bestFret - anchor))
                {
                    bestString = s;
                    bestFret   = fret;
                }
            }
            if (bestString >= 0)
                anchor = bestFret;
            return { bestString, bestFret };
        }

        void writeHook (std::ostringstream& out, const SongStarter& starter, Tick sectionStart, Tick sectionEnd)
        {
            std::vector<MelodyNote> notes;
            for (const auto& note : starter.hook)
                if (note.start >= sectionStart && note.start < sectionEnd)
                    notes.push_back (note);
            if (notes.empty ())
                return;

            const Tick bar       = barTicks (starter.timeSig);
            const auto firstBar  = (notes.front ().start - sectionStart) / bar;
            const auto lastBar   = (notes.back ().start - sectionStart) / bar;
            const int  stepCount = static_cast<int> (bar / kHookStepTicks);

            std::vector<Lines> bars;
            int                anchor = 5;
            for (Tick b = firstBar; b <= lastBar; ++b)
            {
                std::vector<std::array<std::string, kNumStrings>> columns (static_cast<std::size_t> (stepCount));
                std::size_t                                       width = 1;
                for (const auto& note : notes)
                {
                    const Tick offset = note.start - sectionStart - b * bar;
                    if (offset < 0 || offset >= bar)
                        continue;
                    const auto [string, fret] = hookPosition (note.pitch, starter.tuning, starter.capo, anchor);
                    if (string < 0)
                        continue;
                    auto& cell = columns[static_cast<std::size_t> (offset / kHookStepTicks)][static_cast<std::size_t> (kNumStrings - 1 - string)];
                    cell       = std::to_string (fret);
                    width      = std::max (width, cell.size ());
                }

                Lines lines;
                for (auto& line : lines)
                    line = "-";
                for (const auto& column : columns)
                    for (std::size_t line = 0; line < kNumStrings; ++line)
                        lines[line] += pad (column[line], width) + "-";
                bars.push_back (lines);
            }

            out << "Hook (bars " << (firstBar + 1) << "-" << (lastBar + 1) << "):\n";
            writeSystem (out, bars, starter.tuning);
        }

        std::string fingerText (const Voicing& voicing)
        {
            std::string text;
            for (std::size_t s = 0; s < kNumStrings; ++s)
                text += voicing.frets[s] == kMutedString || voicing.fingers[s] == 0 ? '-' : static_cast<char> ('0' + voicing.fingers[s]);
            return text;
        }
    }

    std::string tabBar (const Bar& bar, const StrumPattern& strum, const Tuning& tuning)
    {
        std::ostringstream out;
        writeSystem (out, { barCells (bar, strum) }, tuning);
        return out.str ();
    }

    std::string chordDiagram (const Voicing& voicing)
    {
        std::string text;
        for (int fret : voicing.frets)
        {
            if (fret == kMutedString)
                text += 'x';
            else if (fret < 10)
                text += std::to_string (fret);
            else
                text += "[" + std::to_string (fret) + "]";
        }
        return text;
    }

    std::string writeTab (const SongStarter& starter, const DataSet& data)
    {
        std::ostringstream out;
        const auto         starts = sectionStarts (starter);

        out << "Algo-Rythm song starter | Seed " << starter.seed << " | " << difficultyName (starter.difficulty) << " | " << starter.form << '\n';
        out << "Tuning: " << starter.tuning.name;
        if (starter.capo > 0)
            out << " | Capo: " << starter.capo;
        out << "\n\n";

        for (std::size_t i = 0; i < starter.sections.size (); ++i)
        {
            const auto& section = starter.sections[i];
            out << "[" << section.name << "] " << starter.bpm << " BPM | Key: " << keyText (starter.key) << " | " << starter.timeSig.numerator << "/" << starter.timeSig.denominator << '\n';

            out << "Chords: |";
            for (const auto& bar : section.bars)
            {
                for (const auto& chord : bar.chords)
                    out << ' ' << chordName (chord, starter.key, data) << " (" << numeralText (chord) << ")";
                out << " |";
            }
            out << '\n';
            out << "Strum: " << strumText (section.strum) << '\n';
            out << "Count: " << strumRuler (section.strum, starter.timeSig) << "\n\n";

            std::vector<Lines> bars;
            for (const auto& bar : section.bars)
                bars.push_back (barCells (bar, section.strum));
            writeSystem (out, bars, starter.tuning);

            writeHook (out, starter, starts[i], starts[i + 1]);

            // One diagram for each new chord and voicing of the section.
            std::set<std::pair<std::string, std::string>> seen;
            std::vector<std::string>                      diagrams;
            for (const auto& bar : section.bars)
            {
                for (std::size_t c = 0; c < bar.chords.size () && c < bar.voicings.size (); ++c)
                {
                    const auto name    = chordName (bar.chords[c], starter.key, data);
                    const auto diagram = chordDiagram (bar.voicings[c]);
                    if (!seen.insert ({ name, diagram }).second)
                        continue;

                    std::string line = name;
                    line.resize (std::max<std::size_t> (8, name.size () + 1), ' ');
                    diagrams.push_back (line + diagram + "  fingers " + fingerText (bar.voicings[c]));
                }
            }
            out << "Chord shapes (low E to high E):\n";
            for (const auto& diagram : diagrams)
                out << "  " << diagram << '\n';
            out << '\n';
        }
        return out.str ();
    }
}

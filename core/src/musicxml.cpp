#include "algo/MusicXml.h"

#include "algo/Fretboard.h"
#include "algo/Json.h"
#include "algo/Strum.h"
#include "algo/Pitch.h"
#include "algo/Render.h"
#include "algo/Version.h"
#include "algo/Voicing.h"

#include <algorithm>
#include <map>
#include <sstream>

namespace algo
{
    namespace
    {
        constexpr int kDivisions = 4; // Divisions of a quarter note, so one division is a sixteenth note

        struct Spelling
        {
            char step { 'C' };
            int  alter { 0 };
            int  octave { 4 };
        };

        Spelling spell (MidiNote note, const Key& key)
        {
            const auto name = pitchClassName (pitchClassOf (note), key);
            Spelling   spelling;
            spelling.step = name[0];
            for (std::size_t i = 1; i < name.size (); ++i)
                spelling.alter += name[i] == '#' ? 1 : (name[i] == 'b' ? -1 : 0);

            spelling.octave = note / 12 - 1;
            if (spelling.step == 'C' && spelling.alter < 0)
                ++spelling.octave;
            if (spelling.step == 'B' && spelling.alter > 0)
                --spelling.octave;
            return spelling;
        }

        std::string escape (const std::string& text)
        {
            std::string out;
            for (char c : text)
            {
                switch (c)
                {
                    case '&': out += "&amp;"; break;
                    case '<': out += "&lt;"; break;
                    case '>': out += "&gt;"; break;
                    case '"': out += "&quot;"; break;
                    default: out += c;
                }
            }
            return out;
        }

        // The note type and the dots of a duration in divisions. Returns false for a duration that needs a tie.
        bool noteType (int divisions, std::string& type, int& dots)
        {
            static const std::map<int, std::pair<const char*, int>> types {
                { 1, { "16th", 0 } }, { 2, { "eighth", 0 } }, { 3, { "eighth", 1 } }, { 4, { "quarter", 0 } }, { 6, { "quarter", 1 } }, { 8, { "half", 0 } }, { 12, { "half", 1 } }, { 16, { "whole", 0 } }
            };
            const auto it = types.find (divisions);
            if (it == types.end ())
                return false;
            type = it->second.first;
            dots = it->second.second;
            return true;
        }

        // Divides a duration into values that have a note type, the largest first.
        std::vector<int> splitDuration (int divisions)
        {
            std::vector<int> parts;
            for (int value : { 16, 12, 8, 6, 4, 3, 2, 1 })
                while (divisions >= value)
                {
                    parts.push_back (value);
                    divisions -= value;
                }
            return parts;
        }

        std::string harmonyKind (const std::string& chord)
        {
            static const std::map<std::string, std::string> kinds {
                { "Major", "major" }, { "Minor", "minor" }, { "Aug", "augmented" }, { "Dim", "diminished" }, { "Maj7", "major-seventh" }, { "M7", "minor-seventh" }, { "Dom7", "dominant" }, { "Dim7", "diminished-seventh" }, { "M7b5", "half-diminished" }, { "Sus2", "suspended-second" }, { "Sus4", "suspended-fourth" }, { "Maj6", "major-sixth" }, { "M6", "minor-sixth" }, { "Dom9", "dominant-ninth" }, { "M9", "minor-ninth" }, { "Power", "power" }, { "mMaj7", "major-minor" }
            };
            const auto it = kinds.find (chord);
            return it == kinds.end () ? "other" : it->second;
        }

        class Writer
        {
        public:
            Writer (const SongStarter& starter, const DataSet& data)
                : m_starter (starter),
                  m_data (data),
                  m_fretboard (starter.tuning, starter.capo)
            {
            }

            std::string write ()
            {
                m_out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
                m_out << "<!DOCTYPE score-partwise PUBLIC \"-//Recordare//DTD MusicXML 4.0 Partwise//EN\" \"http://www.musicxml.org/dtds/partwise.dtd\">\n";
                m_out << "<score-partwise version=\"4.0\">\n";
                m_out << "  <work><work-title>" << escape (pitchClassName (m_starter.key.root, m_starter.key) + " " + m_starter.key.scale + " song starter") << "</work-title></work>\n";
                m_out << "  <identification><encoding><software>Algo-Rythm " << escape (version ()) << "</software></encoding></identification>\n";
                m_out << "  <part-list>\n";
                m_out << "    <score-part id=\"P1\"><part-name>Guitar</part-name><score-instrument id=\"P1-I1\"><instrument-name>Steel String Guitar</instrument-name></score-instrument><midi-instrument id=\"P1-I1\"><midi-channel>1</midi-channel><midi-program>" << (kGuitarProgram + 1) << "</midi-program></midi-instrument></score-part>\n";
                if (!m_starter.hook.empty ())
                    m_out << "    <score-part id=\"P2\"><part-name>Hook</part-name><score-instrument id=\"P2-I1\"><instrument-name>Piano</instrument-name></score-instrument><midi-instrument id=\"P2-I1\"><midi-channel>2</midi-channel><midi-program>" << (kHookProgram + 1) << "</midi-program></midi-instrument></score-part>\n";
                m_out << "  </part-list>\n";

                writeGuitarPart ();
                if (!m_starter.hook.empty ())
                    writeHookPart ();

                m_out << "</score-partwise>\n";
                return m_out.str ();
            }

        private:
            int measureDivisions () const
            {
                return static_cast<int> (barTicks (m_starter.timeSig) * kDivisions / kPpq);
            }

            void writeAttributes (bool guitar)
            {
                const auto& t = m_starter.timeSig;
                m_out << "      <attributes>\n";
                m_out << "        <divisions>" << kDivisions << "</divisions>\n";
                m_out << "        <key><fifths>" << keySignatureSharps (m_starter.key, m_data) << "</fifths><mode>" << (isMinorKey (m_starter.key, m_data) ? "minor" : "major") << "</mode></key>\n";
                m_out << "        <time><beats>" << t.numerator << "</beats><beat-type>" << t.denominator << "</beat-type></time>\n";
                if (guitar)
                {
                    m_out << "        <staves>2</staves>\n";
                    m_out << "        <clef number=\"1\"><sign>G</sign><line>2</line><clef-octave-change>-1</clef-octave-change></clef>\n";
                    m_out << "        <clef number=\"2\"><sign>TAB</sign><line>5</line></clef>\n";
                    m_out << "        <staff-details number=\"2\">\n";
                    m_out << "          <staff-lines>6</staff-lines>\n";
                    for (int s = 0; s < kNumStrings; ++s)
                    {
                        const auto open = spell (m_starter.tuning.strings[static_cast<std::size_t> (s)], Key { 0, "Major" });
                        m_out << "          <staff-tuning line=\"" << (s + 1) << "\"><tuning-step>" << open.step << "</tuning-step>";
                        if (open.alter != 0)
                            m_out << "<tuning-alter>" << open.alter << "</tuning-alter>";
                        m_out << "<tuning-octave>" << open.octave << "</tuning-octave></staff-tuning>\n";
                    }
                    if (m_starter.capo > 0)
                        m_out << "          <capo>" << m_starter.capo << "</capo>\n";
                    m_out << "        </staff-details>\n";
                }
                else
                {
                    m_out << "        <clef><sign>G</sign><line>2</line></clef>\n";
                }
                m_out << "      </attributes>\n";
            }

            void writeTempo ()
            {
                m_out << "      <direction placement=\"above\"><direction-type><metronome><beat-unit>quarter</beat-unit><per-minute>" << m_starter.bpm << "</per-minute></metronome></direction-type><sound tempo=\"" << m_starter.bpm << "\"/></direction>\n";
            }

            void writeRehearsal (const std::string& name)
            {
                m_out << "      <direction placement=\"above\"><direction-type><rehearsal>" << escape (name) << "</rehearsal></direction-type></direction>\n";
            }

            void writeHarmony (const ChordSymbol& symbol, int staff)
            {
                const auto chord = concreteChord (symbol, m_starter.key, m_data);
                const auto root  = spell (60 + chord.root, m_starter.key);

                m_out << "      <harmony print-frame=\"no\"><root><root-step>" << root.step << "</root-step>";
                if (root.alter != 0)
                    m_out << "<root-alter>" << root.alter << "</root-alter>";
                m_out << "</root><kind text=\"" << escape (chord.definition.symbol) << "\">" << harmonyKind (chord.definition.name) << "</kind>";
                if (chord.bass)
                {
                    const auto bass = spell (60 + *chord.bass, m_starter.key);
                    m_out << "<bass><bass-step>" << bass.step << "</bass-step>";
                    if (bass.alter != 0)
                        m_out << "<bass-alter>" << bass.alter << "</bass-alter>";
                    m_out << "</bass>";
                }
                m_out << "<staff>" << staff << "</staff></harmony>\n";
            }

            void writeRest (int duration, int voice, int staff, bool measure = false)
            {
                for (int part : measure ? std::vector<int> { duration } : splitDuration (duration))
                {
                    std::string type;
                    int         dots = 0;
                    m_out << "      <note>" << (measure ? "<rest measure=\"yes\"/>" : "<rest/>") << "<duration>" << part << "</duration><voice>" << voice << "</voice>";
                    if (!measure && noteType (part, type, dots))
                    {
                        m_out << "<type>" << type << "</type>";
                        for (int d = 0; d < dots; ++d)
                            m_out << "<dot/>";
                    }
                    if (staff > 0)
                        m_out << "<staff>" << staff << "</staff>";
                    m_out << "</note>\n";
                }
            }

            // One note of a chord or a melody. string is 1 for the high E string, or 0 for no tab position.
            void writeNote (MidiNote pitch, int duration, int voice, int staff, bool chord, bool mute, int string, int fret, bool tieStart = false, bool tieStop = false)
            {
                const auto  spelling = spell (pitch, m_starter.key);
                std::string type;
                int         dots = 0;
                noteType (duration, type, dots);

                m_out << "      <note>";
                if (chord)
                    m_out << "<chord/>";
                m_out << "<pitch><step>" << spelling.step << "</step>";
                if (spelling.alter != 0)
                    m_out << "<alter>" << spelling.alter << "</alter>";
                m_out << "<octave>" << spelling.octave << "</octave></pitch>";
                m_out << "<duration>" << duration << "</duration>";
                if (tieStop)
                    m_out << "<tie type=\"stop\"/>";
                if (tieStart)
                    m_out << "<tie type=\"start\"/>";
                m_out << "<voice>" << voice << "</voice>";
                if (!type.empty ())
                {
                    m_out << "<type>" << type << "</type>";
                    for (int d = 0; d < dots; ++d)
                        m_out << "<dot/>";
                }
                if (mute)
                    m_out << "<notehead>x</notehead>";
                if (staff > 0)
                    m_out << "<staff>" << staff << "</staff>";

                const bool technical = string > 0;
                if (technical || tieStart || tieStop)
                {
                    m_out << "<notations>";
                    if (tieStop)
                        m_out << "<tied type=\"stop\"/>";
                    if (tieStart)
                        m_out << "<tied type=\"start\"/>";
                    if (technical)
                        m_out << "<technical><string>" << string << "</string><fret>" << fret << "</fret></technical>";
                    m_out << "</notations>";
                }
                m_out << "</note>\n";
            }

            void writeStaff (const Bar& bar, const StrumPattern& strum, int staff)
            {
                const int steps    = std::max (1, strum.steps);
                const int slotDivs = std::max (1, measureDivisions () / steps);
                const int voice    = staff == 1 ? 1 : 5;

                for (int slot = 0; slot < steps && slot < static_cast<int> (strum.slots.size ()); ++slot)
                {
                    const auto stroke = strum.slots[static_cast<std::size_t> (slot)];
                    if (bar.voicings.empty () || stroke == Stroke::Rest || stroke == Stroke::Chuck)
                    {
                        writeRest (slotDivs, voice, staff);
                        continue;
                    }

                    const auto  index   = std::min (bar.voicings.size () - 1, static_cast<std::size_t> (slot) * bar.voicings.size () / static_cast<std::size_t> (steps));
                    const auto& voicing = bar.voicings[index];
                    const int   first   = kNumStrings - std::clamp (strum.strings, 1, kNumStrings);
                    const int   plucked = stroke == Stroke::Pluck ? pluckStringIndex (voicing, pluckAt (strum, static_cast<std::size_t> (slot))) : -1;

                    bool any = false;
                    for (int s = first; s < kNumStrings; ++s)
                    {
                        const int fret = voicing.frets[static_cast<std::size_t> (s)];
                        if (fret == kMutedString || (stroke == Stroke::Pluck && s != plucked))
                            continue;
                        writeNote (m_fretboard.midiNoteAt (s, fret), slotDivs, voice, staff, any, stroke == Stroke::Mute, kNumStrings - s, fret);
                        any = true;
                    }
                    if (!any)
                        writeRest (slotDivs, voice, staff);
                }
            }

            void writeGuitarPart ()
            {
                m_out << "  <part id=\"P1\">\n";
                int number = 1;
                for (const auto& section : m_starter.sections)
                {
                    for (std::size_t b = 0; b < section.bars.size (); ++b)
                    {
                        const auto& bar = section.bars[b];
                        m_out << "    <measure number=\"" << number << "\">\n";
                        if (number == 1)
                        {
                            writeAttributes (true);
                            writeTempo ();
                        }
                        if (b == 0)
                            writeRehearsal (section.name);

                        for (const auto& chord : bar.chords)
                            writeHarmony (chord, 1);

                        writeStaff (bar, section.strum, 1);
                        m_out << "      <backup><duration>" << measureDivisions () << "</duration></backup>\n";
                        writeStaff (bar, section.strum, 2);
                        m_out << "    </measure>\n";
                        ++number;
                    }
                }
                m_out << "  </part>\n";
            }

            void writeHookPart ()
            {
                const Tick bar              = barTicks (m_starter.timeSig);
                const Tick total            = sectionStarts (m_starter).back ();
                const auto ticksPerDivision = kPpq / kDivisions;

                m_out << "  <part id=\"P2\">\n";
                int number = 1;
                for (Tick start = 0; start < total; start += bar, ++number)
                {
                    m_out << "    <measure number=\"" << number << "\">\n";
                    if (number == 1)
                        writeAttributes (false);

                    const Tick end    = start + bar;
                    Tick       cursor = start;
                    bool       any    = false;
                    for (const auto& note : m_starter.hook)
                    {
                        const Tick noteEnd = note.start + note.length;
                        if (noteEnd <= start || note.start >= end)
                            continue;

                        const Tick from = std::max ({ note.start, start, cursor });
                        const Tick to   = std::min (noteEnd, end);
                        if (to <= from)
                            continue;

                        if (from > cursor)
                            writeRest (static_cast<int> ((from - cursor) / ticksPerDivision), 1, 0);

                        const auto parts    = splitDuration (static_cast<int> ((to - from) / ticksPerDivision));
                        const bool fromPrev = note.start < start;
                        const bool toNext   = noteEnd > end;
                        for (std::size_t p = 0; p < parts.size (); ++p)
                            writeNote (note.pitch, parts[p], 1, 0, false, false, 0, 0, p + 1 < parts.size () || toNext, p > 0 || fromPrev);

                        cursor = to;
                        any    = true;
                    }

                    if (!any)
                        writeRest (static_cast<int> (bar / ticksPerDivision), 1, 0, true);
                    else if (cursor < end)
                        writeRest (static_cast<int> ((end - cursor) / ticksPerDivision), 1, 0);

                    m_out << "    </measure>\n";
                }
                m_out << "  </part>\n";
            }

            const SongStarter& m_starter;
            const DataSet&     m_data;
            Fretboard          m_fretboard;
            std::ostringstream m_out;
        };
    }

    std::string writeMusicXml (const SongStarter& starter, const DataSet& data)
    {
        return Writer (starter, data).write ();
    }
}

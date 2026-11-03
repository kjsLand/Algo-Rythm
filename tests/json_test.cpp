#include "algo/Json.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstdint>
#include <limits>
#include <stdexcept>

using namespace algo;
using Catch::Matchers::ContainsSubstring;

namespace
{
    // JSON text to object to JSON text must give the same text.
    template<typename T>
    void requireRoundTrip (const T& value)
    {
        const std::string text = Json (value).dump ();

        const T parsed = Json::parse (text).template get<T> ();

        REQUIRE (parsed == value);
        REQUIRE (Json (parsed).dump () == text);
    }

    StrumPattern makeStrum ()
    {
        StrumPattern strum;
        strum.steps   = 8;
        strum.slots   = { Stroke::Down, Stroke::Rest, Stroke::Down, Stroke::Up, Stroke::Mute, Stroke::Up, Stroke::Chuck, Stroke::Up };
        strum.accent  = { 0.6, 0.0, 1.0, 0.6, 0.6, 0.6, 1.0, 0.6 };
        strum.strings = 4;
        return strum;
    }

    ChordSymbol makeChord ()
    {
        ChordSymbol chord;
        chord.degree         = 7;
        chord.accidental     = -1;
        chord.quality        = "maj7";
        chord.bassDegree     = 2;
        chord.bassAccidental = 1;
        chord.tags           = { "borrowed" };
        return chord;
    }

    Voicing makeVoicing ()
    {
        Voicing voicing;
        voicing.frets   = { kMutedString, 3, 2, 0, 1, 0 };
        voicing.fingers = { 0, 3, 2, 0, 1, 0 };
        return voicing;
    }

    SongStarter makeStarter ()
    {
        SongStarter starter;
        starter.seed       = std::numeric_limits<std::uint64_t>::max ();
        starter.key        = Key { 7, "Dorian" };
        starter.bpm        = 96;
        starter.timeSig    = TimeSig { 3, 4 };
        starter.capo       = 2;
        starter.tuning     = Tuning { "Drop D", { 38, 45, 50, 55, 59, 64 } };
        starter.difficulty = Difficulty::Advanced;
        starter.form       = "Loop";

        Bar bar;
        bar.chords   = { makeChord () };
        bar.voicings = { makeVoicing () };

        Section section;
        section.name  = "Chorus";
        section.bars  = { bar, bar };
        section.strum = makeStrum ();

        starter.sections = { section };
        starter.hook     = { MelodyNote { 67, 480, 240, 90 } };
        return starter;
    }
}

TEST_CASE ("Each value type survives a JSON round trip")
{
    SECTION ("Default objects")
    {
        requireRoundTrip (Key {});
        requireRoundTrip (TimeSig {});
        requireRoundTrip (ChordSymbol {});
        requireRoundTrip (FretPos {});
        requireRoundTrip (Voicing {});
        requireRoundTrip (StrumPattern {});
        requireRoundTrip (MelodyNote {});
        requireRoundTrip (Bar {});
        requireRoundTrip (Section {});
        requireRoundTrip (Tuning {});
        requireRoundTrip (SongStarter {});
    }

    SECTION ("Objects with no default values")
    {
        requireRoundTrip (Key { 11, "Natural Minor" });
        requireRoundTrip (TimeSig { 6, 8 });
        requireRoundTrip (makeChord ());
        requireRoundTrip (FretPos { 5, 12 });
        requireRoundTrip (makeVoicing ());
        requireRoundTrip (makeStrum ());
        requireRoundTrip (MelodyNote { 72, 1920, 720, 64 });
        requireRoundTrip (Tuning { "Drop D", { 38, 45, 50, 55, 59, 64 } });
        requireRoundTrip (makeStarter ());
    }
}

TEST_CASE ("A Stroke is one character")
{
    REQUIRE (Json (Stroke::Down) == "D");
    REQUIRE (Json (Stroke::Up) == "U");
    REQUIRE (Json (Stroke::Rest) == "-");
    REQUIRE (Json (Stroke::Mute) == "x");
    REQUIRE (Json (Stroke::Chuck) == "c");

    REQUIRE (Json ("c").get<Stroke> () == Stroke::Chuck);
    REQUIRE_THROWS_AS (Json ("Q").get<Stroke> (), std::invalid_argument);
    REQUIRE_THROWS_AS (Json ("DU").get<Stroke> (), std::invalid_argument);
}

TEST_CASE ("A StrumPattern writes its slots as one string")
{
    REQUIRE (Json (makeStrum ()).at ("slots") == "D-DUxUcU");
}

TEST_CASE ("A StrumPattern with the wrong number of slots does not parse")
{
    Json j         = makeStrum ();
    j.at ("steps") = 16;

    REQUIRE_THROWS_AS (j.get<StrumPattern> (), std::invalid_argument);
}

TEST_CASE ("A muted string in Voicing.frets is -1")
{
    REQUIRE (Json (makeVoicing ()).at ("frets").at (0) == -1);
}

TEST_CASE ("A Voicing with too many strings does not parse")
{
    Json j = makeVoicing ();
    j.at ("frets").push_back (0);

    REQUIRE_THROWS_AS (j.get<Voicing> (), std::invalid_argument);
}

TEST_CASE ("A SongStarter keeps its field order and starts with schemaVersion")
{
    REQUIRE (Json (makeStarter ()).begin ().key () == "schemaVersion");
}

TEST_CASE ("A SongStarter with a larger schemaVersion stops with an error")
{
    Json j                 = makeStarter ();
    j.at ("schemaVersion") = kSchemaVersion + 1;
    j.erase ("sections"); // schemaVersion is read first, so this field is not reached

    REQUIRE_THROWS_WITH (j.get<SongStarter> (), ContainsSubstring ("schemaVersion 2"));
}

TEST_CASE ("A SongStarter with no schemaVersion stops with an error")
{
    Json j = makeStarter ();
    j.erase ("schemaVersion");

    REQUIRE_THROWS_WITH (j.get<SongStarter> (), ContainsSubstring ("no schemaVersion"));
}

TEST_CASE ("A SongStarter with an unknown difficulty does not parse")
{
    Json j              = makeStarter ();
    j.at ("difficulty") = "Expert";

    REQUIRE_THROWS_WITH (j.get<SongStarter> (), ContainsSubstring ("Expert"));
}

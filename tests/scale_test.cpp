#include "algo/Pitch.h"
#include "algo/Scale.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <set>
#include <stdexcept>
#include <vector>

using namespace algo;

TEST_CASE ("scalePitchClasses gives the notes in degree order")
{
    REQUIRE (scalePitchClasses (Key { 0, "Major" }) == std::vector<PitchClass> { 0, 2, 4, 5, 7, 9, 11 });
    REQUIRE (scalePitchClasses (Key { 9, "Minor Pentatonic" }) == std::vector<PitchClass> { 9, 0, 2, 4, 7 });
    REQUIRE (scalePitchClasses (Key { 5, "Major" }) == std::vector<PitchClass> { 5, 7, 9, 10, 0, 2, 4 });
}

TEST_CASE ("An unknown scale throws std::out_of_range")
{
    REQUIRE_THROWS_AS (scalePitchClasses (Key { 0, "Klingon" }), std::out_of_range);
    REQUIRE_THROWS_AS (isInScale (0, Key { 0, "Klingon" }), std::out_of_range);
}

TEST_CASE ("A caller can give other scale data")
{
    DataSet data;
    data.scales = ScaleRegistry::fromJsonText (R"({ "schemaVersion": 1, "scales": [
        { "name": "Tritone", "steps": [6, 6], "tags": [] } ] })");

    REQUIRE (scalePitchClasses (Key { 1, "Tritone" }, data) == std::vector<PitchClass> { 1, 7 });
    REQUIRE_THROWS_AS (scalePitchClasses (Key { 0, "Major" }, data), std::out_of_range);
}

TEST_CASE ("The scale functions agree for all 15 scales in all 12 keys")
{
    const auto& scales = bundledData ().scales.all ();
    REQUIRE (scales.size () == 15);

    for (const auto& scale : scales)
        for (PitchClass root = 0; root < 12; ++root)
        {
            CAPTURE (scale.name, root);

            const Key  key { root, scale.name };
            const auto pcs = scalePitchClasses (key);

            REQUIRE (pcs.size () == scale.steps.size ());
            REQUIRE (pcs[0] == root);
            REQUIRE (std::set<PitchClass> (pcs.begin (), pcs.end ()).size () == pcs.size ());

            for (std::size_t i = 0; i < pcs.size (); ++i)
                REQUIRE (scaleDegreeOf (pcs[i], key) == static_cast<int> (i) + 1);

            const std::set<PitchClass> inScale (pcs.begin (), pcs.end ());
            for (PitchClass pc = 0; pc < 12; ++pc)
            {
                CAPTURE (pc);
                REQUIRE (isInScale (pc, key) == (inScale.count (pc) > 0));
                REQUIRE (scaleDegreeOf (pc, key).has_value () == (inScale.count (pc) > 0));
            }
        }
}

TEST_CASE ("scaleDegreeOf gives no value for a note out of the scale")
{
    const Key cMajor { 0, "Major" };

    REQUIRE (scaleDegreeOf (0, cMajor) == 1);
    REQUIRE (scaleDegreeOf (11, cMajor) == 7);
    REQUIRE_FALSE (scaleDegreeOf (1, cMajor).has_value ());
    REQUIRE (scaleDegreeOf (12, cMajor) == 1);
}

TEST_CASE ("nearestScaleNote returns a note in the scale with no change")
{
    REQUIRE (nearestScaleNote (60, Key { 0, "Major" }) == 60);
    REQUIRE (nearestScaleNote (62, Key { 0, "Major" }) == 62);
}

TEST_CASE ("nearestScaleNote returns the lower note if two notes have the same distance")
{
    REQUIRE (nearestScaleNote (61, Key { 0, "Major" }) == 60);
    REQUIRE (nearestScaleNote (63, Key { 0, "Major" }) == 62);
}

TEST_CASE ("nearestScaleNote can move across a large gap")
{
    const Key aMinorPentatonic { 9, "Minor Pentatonic" }; // A C D E G

    REQUIRE (nearestScaleNote (70, aMinorPentatonic) == 69); // A#4 to A4
    REQUIRE (nearestScaleNote (71, aMinorPentatonic) == 72); // B4 to C5
    REQUIRE (nearestScaleNote (66, aMinorPentatonic) == 67); // F#4 to G4
}

TEST_CASE ("nearestScaleNote finds the closest scale note for all scales, keys and notes")
{
    for (const auto& scale : bundledData ().scales.all ())
        for (PitchClass root = 0; root < 12; ++root)
            for (MidiNote note = 0; note <= 127; ++note)
            {
                CAPTURE (scale.name, root, note);

                const Key      key { root, scale.name };
                const MidiNote nearest = nearestScaleNote (note, key);
                const int      dist    = std::abs (nearest - note);

                REQUIRE (isInScale (nearest, key));
                for (int closer = 0; closer < dist; ++closer)
                {
                    REQUIRE_FALSE (isInScale (note - closer, key));
                    REQUIRE_FALSE (isInScale (note + closer, key));
                }
                if (nearest > note)
                    REQUIRE_FALSE (isInScale (note - dist, key));
            }
}

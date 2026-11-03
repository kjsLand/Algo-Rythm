#include "algo/GenerationError.h"
#include "algo/VoicingCache.h"

#include "support/VoicingHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace algo;
using algo::test::chordOf;
using algo::test::textOf;
using algo::test::voicingOf;
using Catch::Matchers::ContainsSubstring;

namespace
{
    std::vector<std::vector<Voicing>> voicingLists (const std::vector<std::vector<std::string>>& texts)
    {
        std::vector<std::vector<Voicing>> lists;
        for (const auto& list : texts)
        {
            lists.emplace_back ();
            for (const auto& text : list)
                lists.back ().push_back (voicingOf (text));
        }
        return lists;
    }

    // Returns the total cost and the frets of all paths.
    std::vector<std::pair<int, std::string>> allPaths (const std::vector<std::vector<Voicing>>& candidates, Difficulty difficulty)
    {
        std::vector<std::pair<int, std::string>> paths;
        std::vector<const Voicing*>              path;

        const std::function<void (std::size_t, int)> visit = [&] (std::size_t step, int cost)
        {
            if (step == candidates.size ())
            {
                std::string text;
                for (const auto* voicing : path)
                    text += textOf (*voicing) + " ";
                paths.push_back ({ cost, text });
                return;
            }

            for (const auto& candidate : candidates[step])
            {
                int added = staticCost (candidate, difficulty);
                if (!path.empty ())
                    added += transitionCost (*path.back (), candidate);

                path.push_back (&candidate);
                visit (step + 1, cost + added);
                path.pop_back ();
            }
        };
        visit (0, 0);

        std::stable_sort (paths.begin (), paths.end (), [] (const auto& a, const auto& b)
        {
            return a.first < b.first;
        });
        return paths;
    }

    std::string textOf (const VoicingSequence& sequence)
    {
        std::string text;
        for (const auto& voicing : sequence.voicings)
            text += algo::test::textOf (voicing) + " ";
        return text;
    }
}

TEST_CASE ("For G, D, Em, C at Beginner, the best sequence uses only open shapes")
{
    const std::vector<ConcreteChord> chords { chordOf (7, "Major"), chordOf (2, "Major"), chordOf (4, "Minor"), chordOf (0, "Major") };

    VoicingCache                      cache;
    std::vector<std::vector<Voicing>> candidates;
    for (const auto& chord : chords)
        candidates.push_back (cache.candidates (chord, Difficulty::Beginner, Fretboard {}));

    const auto best = chooseSequence (chords, candidates, Difficulty::Beginner).front ();

    REQUIRE (textOf (best) == "320003 xx0232 022000 x32010 ");
}

TEST_CASE ("The function returns 5 sequences when 5 or more paths exist")
{
    const std::vector<ConcreteChord> chords { chordOf (0, "Major"), chordOf (7, "Major") };

    SECTION ("6 paths give 5 sequences")
    {
        const auto sequences = chooseSequence (chords, voicingLists ({ { "x32010", "x35553" }, { "320003", "355433", "xx5433" } }), Difficulty::Beginner);
        REQUIRE (sequences.size () == 5);
    }

    SECTION ("4 paths give 4 sequences")
    {
        const auto sequences = chooseSequence (chords, voicingLists ({ { "x32010", "x35553" }, { "320003", "355433" } }), Difficulty::Beginner);
        REQUIRE (sequences.size () == 4);
    }

    SECTION ("The sequences are different and in order of total cost")
    {
        VoicingCache cache;
        const auto   sequences = chooseSequence (chords, { cache.candidates (chords[0], Difficulty::Advanced, Fretboard {}), cache.candidates (chords[1], Difficulty::Advanced, Fretboard {}) }, Difficulty::Advanced);

        REQUIRE (sequences.size () == 5);

        std::set<std::string> texts;
        for (std::size_t i = 0; i < sequences.size (); ++i)
        {
            texts.insert (textOf (sequences[i]));
            if (i > 0)
                REQUIRE (sequences[i - 1].cost <= sequences[i].cost);
        }
        REQUIRE (texts.size () == sequences.size ());
    }
}

TEST_CASE ("A brute-force test on 4 chords gives the same best sequences")
{
    const std::vector<ConcreteChord> chords { chordOf (0, "Major"), chordOf (9, "Minor"), chordOf (5, "Major"), chordOf (7, "Major") };

    for (const auto difficulty : { Difficulty::Beginner, Difficulty::Advanced })
    {
        VoicingCache                      cache;
        std::vector<std::vector<Voicing>> candidates;
        for (const auto& chord : chords)
        {
            const auto& list = cache.candidates (chord, difficulty, Fretboard {});
            REQUIRE (list.size () >= 3);

            // Use a part of a long list, from each area of the neck.
            std::vector<Voicing> some;
            for (std::size_t i = 0; i < list.size (); i += std::max<std::size_t> (list.size () / 12, 1))
                some.push_back (list[i]);
            candidates.push_back (some);
        }

        const auto paths     = allPaths (candidates, difficulty);
        const auto sequences = chooseSequence (chords, candidates, difficulty, Fretboard {}, CostWeights {}, 5);

        REQUIRE (sequences.size () == 5);

        for (std::size_t i = 0; i < sequences.size (); ++i)
        {
            CAPTURE (i);
            REQUIRE (sequences[i].cost == paths[i].first);

            // Two paths can have the same cost, so compare with the cost of the path of this sequence.
            const auto text = textOf (sequences[i]);
            const auto path = std::find_if (paths.begin (), paths.end (), [&text] (const auto& item)
            {
                return item.second == text;
            });
            REQUIRE (path != paths.end ());
            REQUIRE (path->first == sequences[i].cost);
        }
    }
}

TEST_CASE ("A chord with no candidate throws GenerationError with the chord name")
{
    const std::vector<ConcreteChord> chords { chordOf (0, "Major"), { 7, bundledData ().chords.get ("Major"), std::nullopt, "G" } };

    REQUIRE_THROWS_AS (chooseSequence (chords, voicingLists ({ { "x32010" }, {} }), Difficulty::Beginner), GenerationError);
    REQUIRE_THROWS_WITH (chooseSequence (chords, voicingLists ({ { "x32010" }, {} }), Difficulty::Beginner), ContainsSubstring ("G"));
}

TEST_CASE ("The sequence selection rejects bad arguments")
{
    const std::vector<ConcreteChord> chords { chordOf (0, "Major") };
    const auto                       lists = voicingLists ({ { "x32010" } });

    REQUIRE_THROWS_AS (chooseSequence (chords, {}, Difficulty::Beginner), std::invalid_argument);
    REQUIRE_THROWS_AS (chooseSequence (chords, lists, Difficulty::Beginner, Fretboard {}, CostWeights {}, 0), std::invalid_argument);
}

TEST_CASE ("No chords give no sequences and one chord gives its cheapest candidates")
{
    REQUIRE (chooseSequence ({}, {}, Difficulty::Beginner).empty ());

    const auto sequences = chooseSequence ({ chordOf (0, "Major") }, voicingLists ({ { "x35553", "x32010" } }), Difficulty::Beginner);

    REQUIRE (sequences.size () == 2);
    REQUIRE (textOf (sequences[0]) == "x32010 ");
    REQUIRE (sequences[0].cost == staticCost (voicingOf ("x32010"), Difficulty::Beginner));
}

TEST_CASE ("A change to the weights changes the best sequence")
{
    const std::vector<ConcreteChord> chords { chordOf (0, "Major"), chordOf (0, "Major") };
    const auto                       lists = voicingLists ({ { "x32010" }, { "x32010", "x35553" } });

    CostWeights weights;
    REQUIRE (textOf (chooseSequence (chords, lists, Difficulty::Advanced, Fretboard {}, weights).front ()) == "x32010 x32010 ");

    weights.finger = -100;
    REQUIRE (textOf (chooseSequence (chords, lists, Difficulty::Advanced, Fretboard {}, weights).front ()) == "x32010 x35553 ");
}

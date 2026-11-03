#include "algo/GenerationError.h"
#include "algo/Voicing.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace algo
{
    namespace
    {
        // One of the k best paths that end on a candidate.
        struct Path
        {
            int         cost { 0 };
            std::size_t previous { 0 }; // The candidate of the chord before
            std::size_t rank { 0 };     // The index of the path in the list of that candidate
        };

        using PathLists = std::vector<std::vector<Path>>; // One list for each candidate, with the lowest cost first

        // Puts the path into a sorted list that holds k paths. A path with an equal cost goes after the others.
        void keepBest (std::vector<Path>& list, const Path& path, std::size_t k)
        {
            const auto position = std::upper_bound (list.begin (), list.end (), path, [] (const Path& a, const Path& b)
            {
                return a.cost < b.cost;
            });

            if (position == list.end () && list.size () == k)
                return;

            list.insert (position, path);
            if (list.size () > k)
                list.pop_back ();
        }

        std::vector<int> staticCosts (const std::vector<Voicing>& candidates, Difficulty difficulty, const Fretboard& fretboard, const CostWeights& weights)
        {
            std::vector<int> costs;
            costs.reserve (candidates.size ());
            for (const auto& candidate : candidates)
                costs.push_back (staticCost (candidate, difficulty, fretboard, weights));
            return costs;
        }

        std::vector<TransitionPoint> transitionPoints (const std::vector<Voicing>& candidates, const Fretboard& fretboard)
        {
            std::vector<TransitionPoint> points;
            points.reserve (candidates.size ());
            for (const auto& candidate : candidates)
                points.push_back (transitionPoint (candidate, fretboard));
            return points;
        }
    }

    std::vector<VoicingSequence> chooseSequence (const std::vector<ConcreteChord>& chords, const std::vector<std::vector<Voicing>>& candidates, Difficulty difficulty, const Fretboard& fretboard, const CostWeights& weights, int k)
    {
        if (chords.size () != candidates.size ())
            throw std::invalid_argument ("Each chord must have a list of candidates");
        if (k < 1)
            throw std::invalid_argument ("The number of sequences must be 1 or more");

        for (std::size_t i = 0; i < chords.size (); ++i)
            if (candidates[i].empty ())
                throw GenerationError ("The chord has no voicing: " + chords[i].name);

        if (chords.empty ())
            return {};

        const auto best = static_cast<std::size_t> (k);

        std::vector<PathLists> steps (chords.size ());

        auto costs = staticCosts (candidates[0], difficulty, fretboard, weights);
        for (int cost : costs)
            steps[0].push_back ({ Path { cost, 0, 0 } });

        auto previousPoints = transitionPoints (candidates[0], fretboard);

        for (std::size_t step = 1; step < chords.size (); ++step)
        {
            costs             = staticCosts (candidates[step], difficulty, fretboard, weights);
            const auto points = transitionPoints (candidates[step], fretboard);

            steps[step].resize (points.size ());

            for (std::size_t next = 0; next < points.size (); ++next)
            {
                auto& list = steps[step][next];
                list.reserve (best + 1);

                for (std::size_t previous = 0; previous < previousPoints.size (); ++previous)
                {
                    const int   added = transitionCost (previousPoints[previous], points[next], weights) + costs[next];
                    const auto& paths = steps[step - 1][previous];

                    for (std::size_t rank = 0; rank < paths.size (); ++rank)
                    {
                        const int cost = paths[rank].cost + added;

                        // The list of the previous candidate is sorted, so no later path can enter.
                        if (list.size () == best && cost >= list.back ().cost)
                            break;

                        keepBest (list, { cost, previous, rank }, best);
                    }
                }
            }
            previousPoints = points;
        }

        // The final paths, as a candidate and a rank.
        struct Final
        {
            int         cost;
            std::size_t candidate;
            std::size_t rank;
        };

        std::vector<Final> finals;
        for (std::size_t candidate = 0; candidate < steps.back ().size (); ++candidate)
            for (std::size_t rank = 0; rank < steps.back ()[candidate].size (); ++rank)
                finals.push_back ({ steps.back ()[candidate][rank].cost, candidate, rank });

        std::stable_sort (finals.begin (), finals.end (), [] (const Final& a, const Final& b)
        {
            return a.cost < b.cost;
        });
        if (finals.size () > best)
            finals.resize (best);

        std::vector<VoicingSequence> sequences;
        sequences.reserve (finals.size ());

        for (const auto& end : finals)
        {
            VoicingSequence sequence;
            sequence.cost = end.cost;
            sequence.voicings.resize (chords.size ());

            std::size_t candidate = end.candidate;
            std::size_t rank      = end.rank;

            for (std::size_t step = chords.size (); step-- > 0;)
            {
                sequence.voicings[step] = candidates[step][candidate];

                const auto& path = steps[step][candidate][rank];
                candidate        = path.previous;
                rank             = path.rank;
            }
            sequences.push_back (std::move (sequence));
        }
        return sequences;
    }
}

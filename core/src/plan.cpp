#include "algo/Plan.h"

#include "algo/Diatonic.h"
#include "algo/Fretboard.h"
#include "algo/Pitch.h"
#include "algo/Scale.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace algo
{
    namespace
    {
        // The Beginner keys are G, C, D, A and E major, and A, E and D minor.
        const std::vector<PitchClass> kBeginnerMajorRoots { 7, 0, 2, 9, 4 };
        const std::vector<PitchClass> kBeginnerMinorRoots { 9, 4, 2 };

        Key chooseKey (const GenerationSettings& settings, Rng& rng, const DataSet& data)
        {
            Key key;
            key.scale = settings.scale ? *settings.scale : moodScale (settings.mood);

            if (settings.keyRoot)
            {
                key.root = *settings.keyRoot;
            }
            else if (settings.difficulty == Difficulty::Beginner)
            {
                // The mood or the settings give the scale, so the scale decides which roots are permitted.
                const auto& roots = isMinorScale (key.scale, data) ? kBeginnerMinorRoots : kBeginnerMajorRoots;
                key.root          = roots[static_cast<std::size_t> (rng.uniformInt (0, static_cast<int> (roots.size ()) - 1))];
            }
            else
            {
                key.root = rng.uniformInt (0, 11);
            }
            return key;
        }

        // Odd values go up to the next even value. Validated settings always have an even value in range.
        int chooseBpm (const GenerationSettings& settings, Rng& rng)
        {
            const int bpm = rng.uniformInt (settings.bpmMin, settings.bpmMax);
            if (bpm % 2 == 0)
                return bpm;
            return bpm < settings.bpmMax ? bpm + 1 : bpm - 1;
        }

        bool hasOpenShape (PitchClass root, const std::string& chord, const DataSet& data)
        {
            const Tuning standard;
            return std::any_of (data.shapes.begin (), data.shapes.end (), [&] (const ChordShape& shape)
            {
                if (!shape.open || shape.difficulty != Difficulty::Beginner || shape.chord != chord)
                    return false;

                const auto string = static_cast<std::size_t> (shape.rootStringIndex ());
                return pitchClassOf (standard.strings[string] + *shape.frets[string]) == root;
            });
        }

        // Returns the lowest capo that lets a Beginner play the tonic chord with an open shape.
        // Returns 0 if the tonic chord has an open shape, or if no capo gives one.
        int suggestCapo (const Key& key, const DataSet& data)
        {
            const auto tonic = diatonicTriad (key, 1, data).chord.name;
            if (hasOpenShape (key.root, tonic, data))
                return 0;

            for (int capo = 1; capo <= Fretboard::kMaxCapo; ++capo)
                if (hasOpenShape (pitchClassOf (key.root - capo), tonic, data))
                    return capo;

            return 0;
        }
    }

    std::string moodScale (Mood mood)
    {
        switch (mood)
        {
            case Mood::Bright: return "Major";
            case Mood::Sad: return "Natural Minor";
            case Mood::Hopeful: return "Dorian";
            case Mood::Bluesy: return "Mixolydian";
        }
        throw std::invalid_argument ("Unknown mood value");
    }

    SongPlan planSong (const GenerationSettings& settings, Rng& rng, const DataSet& data)
    {
        const auto errors = validate (settings, data);
        if (!errors.empty ())
            throw std::invalid_argument (errors.front ());

        SongPlan plan;
        plan.key        = chooseKey (settings, rng, data);
        plan.bpm        = chooseBpm (settings, rng);
        plan.timeSig    = settings.timeSig;
        plan.capo       = settings.capo;
        plan.difficulty = settings.difficulty;
        plan.form       = settings.form;

        // A capo from the settings stays.
        if (plan.capo == 0 && plan.difficulty == Difficulty::Beginner)
            plan.capo = suggestCapo (plan.key, data);

        const auto form = std::find_if (data.forms.begin (), data.forms.end (), [&settings] (const FormDefinition& candidate)
        {
            return candidate.name == settings.form;
        });
        plan.sections   = form->sections; // validate has checked the name

        return plan;
    }
}

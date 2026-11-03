#pragma once

#include "algo/Data.h"
#include "algo/Engine.h"
#include "algo/Settings.h"
#include "algo/Types.h"
#include "algo/VoicingCache.h"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>

// The state of the app: the settings, the current song starter and the selection.
// Panels listen for changes. All music logic comes from algo::core (REQ-11-1).
class AppState : public juce::ChangeBroadcaster
{
public:
    AppState ();
    ~AppState () override;

    algo::GenerationSettings& settings ()
    {
        return m_settings;
    }
    const algo::GenerationSettings& settings () const
    {
        return m_settings;
    }

    // Call this after a change to the settings. It does not generate.
    void settingsChanged ();

    // The current song starter, or nullptr before the first generation.
    std::shared_ptr<const algo::SongStarter> starter () const
    {
        return m_starter;
    }

    int selectedSection () const
    {
        return m_section;
    }
    int selectedBar () const
    {
        return m_bar;
    }
    void select (int section, int bar);

    // Runs algo::generate in a background job with a new seed (REQ-11-2). The result of an older job is ignored.
    void requestGenerate ();

    // As requestGenerate, with the given seed.
    void requestGenerate (std::uint64_t seed);

    // Runs algo::reroll for one stage and section in a background job.
    void requestReroll (algo::Stage stage, int section);

    // Shows a song starter, for example from the library.
    void setStarter (algo::SongStarter starter);

    bool isBusy () const
    {
        return m_busy > 0;
    }

    const juce::String& status () const
    {
        return m_status;
    }
    void setStatus (const juce::String& text);

    // The lock of a section item: 0 = chords, 1 = voicings, 2 = strum.
    bool sectionLock (int section, int item) const;
    void setSectionLock (int section, int item, bool locked);

    // "rules" or "learned" (SPEC-15). If the learned files do not exist, the status bar shows a warning.
    void setTables (const std::string& tables);

    // In A/B test mode, each Generate click chooses "rules" or "learned" at random. The UI does not show it (REQ-15-5).
    void setAbTestMode (bool on);
    bool abTestMode () const
    {
        return m_abTest;
    }

    // Sets the rating, saves the song starter to the library and adds a line to ratings.csv.
    void rate (int rating);

    bool leftHanded () const
    {
        return m_leftHanded;
    }
    void setLeftHanded (bool on);

    // The data for the current tables.
    const algo::DataSet& data () const;

    // ~/Algo-Rythm
    static juce::File userFolder ();

private:
    struct DataContext
    {
        algo::DataSet      data;
        algo::VoicingCache cache;
    };

    std::shared_ptr<DataContext> contextFor (const std::string& tables);
    void                         runJob (std::function<algo::SongStarter ()> job);

    algo::GenerationSettings                 m_settings;
    std::shared_ptr<const algo::SongStarter> m_starter;
    int                                      m_section { 0 };
    int                                      m_bar { 0 };
    juce::String                             m_status;
    bool                                     m_abTest { false };
    bool                                     m_leftHanded { false };

    std::shared_ptr<DataContext> m_rules;
    std::shared_ptr<DataContext> m_learned;

    juce::ThreadPool m_pool { 1 };
    std::atomic<int> m_latest { 0 };
    int              m_busy { 0 };
    juce::Random     m_random;

    JUCE_DECLARE_WEAK_REFERENCEABLE (AppState)
    JUCE_DECLARE_NON_COPYABLE (AppState)
};

#pragma once

#include "algo/Settings.h"

#include <juce_data_structures/juce_data_structures.h>

#include <memory>

// The saved app preferences (T14-06): the last settings, the window bounds, the click state,
// the last export folder and the audio device state.
class Preferences
{
public:
    Preferences ();

    // The settings of the last session, or the default settings.
    algo::GenerationSettings settings () const;
    void                     setSettings (const algo::GenerationSettings& settings);

    juce::String windowBounds () const;
    void         setWindowBounds (const juce::String& bounds);

    bool clickOn () const;
    void setClickOn (bool on);

    juce::File lastExportFolder () const;
    void       setLastExportFolder (const juce::File& folder);

    std::unique_ptr<juce::XmlElement> audioState () const;
    void                              setAudioState (const juce::XmlElement* xml);

    bool abTestMode () const;
    void setAbTestMode (bool on);

    bool leftHanded () const;
    void setLeftHanded (bool on);

    void save ();

private:
    juce::PropertiesFile m_file;
};

#pragma once

#include "MainComponent.h"
#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The plugin window: the same panels as the app (REQ-16-1), with a "Preview Sound" toggle.
// The host transport plays the song starter, so the plugin has no transport bar.
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor& owner);

    void paint (juce::Graphics& g) override;
    void resized () override;

private:
    PluginProcessor&   m_processor;
    MainComponent      m_main;
    juce::ToggleButton m_preview { "Preview Sound" };
    juce::Label        m_hint;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

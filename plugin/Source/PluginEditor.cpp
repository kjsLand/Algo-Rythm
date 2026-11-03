#include "PluginEditor.h"

#include "Theme.h"

PluginEditor::PluginEditor (PluginProcessor& owner)
    : juce::AudioProcessorEditor (owner),
      m_processor (owner),
      m_main (owner.state (), nullptr)
{
    addAndMakeVisible (m_main);
    m_main.showMenuBar (true);

    m_preview.setToggleState (owner.previewSound (), juce::dontSendNotification);
    m_preview.setTooltip ("Play the MIDI output with the plucked-string synthesizer");
    m_preview.onClick = [this]
    {
        m_processor.setPreviewSound (m_preview.getToggleState ());
    };
    addAndMakeVisible (m_preview);

    m_hint.setText ("The host transport plays the song starter. Route the MIDI output to an instrument track.", juce::dontSendNotification);
    m_hint.setColour (juce::Label::textColourId, Theme::textDim);
    addAndMakeVisible (m_hint);

    setResizable (true, true);
    setResizeLimits (MainComponent::kMinWidth, MainComponent::kMinHeight + 28, 4000, 3000);
    setSize (1200, 828);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);
}

void PluginEditor::resized ()
{
    auto r   = getLocalBounds ();
    auto top = r.removeFromTop (28).reduced (Theme::gap, 2);
    m_preview.setBounds (top.removeFromLeft (140));
    m_hint.setBounds (top);
    m_main.setBounds (r);
}

#pragma once

#include "Sequencer.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_events/juce_events.h>

#include <memory>

// The audio device, the synthesizer and the sequencer (T12-01). If no device opens, the app still runs
// and errorText says why.
class AudioEngine : private juce::Timer
{
public:
    explicit AudioEngine (const juce::XmlElement* savedState);
    ~AudioEngine () override;

    // Empty if the output device is open.
    const juce::String& errorText () const
    {
        return m_error;
    }

    juce::AudioDeviceManager& deviceManager ()
    {
        return m_devices;
    }
    Sequencer& sequencer ()
    {
        return m_sequencer;
    }

    // Renders the song starter and gives the events to the audio thread.
    void setStarter (const algo::SongStarter& starter, const algo::DataSet& data);

private:
    void timerCallback () override;

    // Prepares the synthesizer and then the sequencer.
    class Source : public juce::AudioSource
    {
    public:
        Source (juce::Synthesiser& synth, Sequencer& sequencer) : m_synth (synth), m_sequencer (sequencer) {}
        void prepareToPlay (int block, double sampleRate) override;
        void releaseResources () override
        {
            m_sequencer.releaseResources ();
        }
        void getNextAudioBlock (const juce::AudioSourceChannelInfo& info) override
        {
            m_sequencer.getNextAudioBlock (info);
        }

    private:
        juce::Synthesiser& m_synth;
        Sequencer&         m_sequencer;
    };

    juce::AudioDeviceManager m_devices;
    juce::Synthesiser        m_synth;
    Sequencer                m_sequencer { m_synth };
    Source                   m_source { m_synth, m_sequencer };
    juce::AudioSourcePlayer  m_player;
    juce::String             m_error;
};

#include "AudioEngine.h"

#include "PluckVoice.h"

void AudioEngine::Source::prepareToPlay (int block, double sampleRate)
{
    prepareSynth (m_synth, sampleRate);
    m_sequencer.prepareToPlay (block, sampleRate);
}

AudioEngine::AudioEngine (const juce::XmlElement* savedState)
{
    configureSynth (m_synth);
    prepareSynth (m_synth, 48000.0);

    // 0 inputs and 2 outputs. The saved state selects the device of the last session (T12-05).
    m_error = m_devices.initialise (0, 2, savedState, true);
    if (m_error.isEmpty () && m_devices.getCurrentAudioDevice () == nullptr)
        m_error = "No audio output device is available.";

    m_player.setSource (&m_source);
    m_devices.addAudioCallback (&m_player);
    startTimerHz (10);
}

AudioEngine::~AudioEngine ()
{
    stopTimer ();
    m_devices.removeAudioCallback (&m_player);
    m_player.setSource (nullptr);
    m_devices.closeAudioDevice ();
}

void AudioEngine::setStarter (const algo::SongStarter& starter, const algo::DataSet& data)
{
    m_sequencer.setSchedule (EventSchedule::fromStarter (starter, data));
}

void AudioEngine::timerCallback ()
{
    m_sequencer.collectGarbage ();
}

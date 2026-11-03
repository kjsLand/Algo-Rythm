#include "PluginProcessor.h"

#include "PluckVoice.h"
#include "PluginEditor.h"

#include "algo/Files.h"
#include "algo/Json.h"

#include <cmath>

PluginProcessor::PluginProcessor ()
    : juce::AudioProcessor (BusesProperties ().withOutput ("Output", juce::AudioChannelSet::stereo (), true))
{
    configureSynth (m_synth);
    prepareSynth (m_synth, m_sampleRate);
    m_state.addChangeListener (this);
    m_state.requestGenerate ();
}

PluginProcessor::~PluginProcessor ()
{
    m_state.removeChangeListener (this);
    m_slot.collectGarbage ();
    delete m_current;
    delete m_waiting;
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet ();
    return out == juce::AudioChannelSet::stereo () || out == juce::AudioChannelSet::mono ();
}

void PluginProcessor::prepareToPlay (double sampleRate, int)
{
    m_sampleRate = sampleRate;
    prepareSynth (m_synth, sampleRate);
}

void PluginProcessor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    m_slot.collectGarbage ();
    const auto starter = m_state.starter ();
    if (starter != nullptr && starter != m_scheduled)
    {
        m_slot.post (EventSchedule::fromStarter (*starter, m_state.data ()));
        m_scheduled = starter;
        updateHostDisplay ();
    }
}

void PluginProcessor::allNotesOff (juce::MidiBuffer& midi, int offset)
{
    for (int channel = 0; channel < 16; ++channel)
        for (int note = 0; note < 128; ++note)
            if (m_sounding[static_cast<std::size_t> (channel)][static_cast<std::size_t> (note)])
            {
                midi.addEvent (juce::MidiMessage::noteOff (channel + 1, note), offset);
                m_sounding[static_cast<std::size_t> (channel)][static_cast<std::size_t> (note)] = false;
            }
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear ();
    midi.clear ();
    const int numSamples = buffer.getNumSamples ();

    // A new song starter replaces the old events. The old schedule goes back to the message thread for deletion.
    if (auto* posted = m_slot.takePosted ())
    {
        if (m_waiting != nullptr && m_slot.retire (m_waiting))
            m_waiting = nullptr;
        if (m_waiting == nullptr)
            m_waiting = posted;
    }
    if (m_waiting != nullptr && m_slot.retire (m_current))
    {
        m_current = m_waiting;
        m_waiting = nullptr;
        if (m_wasPlaying)
            allNotesOff (midi, 0);
    }

    // Read the tempo and the position of the host (REQ-16-2).
    double bpm     = 120.0;
    double ppq     = 0.0;
    bool   playing = false;
    if (auto* head = getPlayHead ())
    {
        if (const auto position = head->getPosition ())
        {
            bpm     = position->getBpm ().orFallback (120.0);
            ppq     = position->getPpqPosition ().orFallback (0.0);
            playing = position->getIsPlaying ();
        }
    }

    if (!playing || m_current == nullptr)
    {
        // The host stopped: end each note that sounds.
        if (m_wasPlaying)
            allNotesOff (midi, 0);
        m_wasPlaying = false;
        m_shownTicks = -1.0;
    }
    else
    {
        const double samplesPerTick = getSampleRate () * 60.0 / (std::max (1.0, bpm) * algo::kPpq);
        const double startTick      = ppq * algo::kPpq;
        const double endTick        = startTick + numSamples / samplesPerTick;

        // A jump of the host position (a loop or a click on the timeline) ends the notes that sound.
        if (m_wasPlaying && std::abs (startTick - m_lastEndTick) > 1.0)
            allNotesOff (midi, 0);

        // The host has its own metronome, so the MIDI output has no click.
        collectSongEvents (*m_current, startTick, endTick, samplesPerTick, numSamples, midi, m_sounding, false);
        m_wasPlaying  = true;
        m_lastEndTick = endTick;
        m_shownTicks  = std::fmod (startTick, std::max (1.0, static_cast<double> (m_current->length)));
    }

    if (m_preview)
        m_synth.renderNextBlock (buffer, midi, 0, numSamples);
}

juce::AudioProcessorEditor* PluginProcessor::createEditor ()
{
    return new PluginEditor (*this);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    algo::Json state;
    state["version"] = 1;
    auto settings    = m_state.settings ();
    settings.previous.reset ();
    state["settings"] = settings;
    state["preview"]  = m_preview.load ();
    if (const auto starter = m_state.starter ())
        state["starter"] = *starter;

    const auto text = state.dump ();
    destData.replaceAll (text.data (), text.size ());
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    try
    {
        const auto state = algo::Json::parse (std::string (static_cast<const char*> (data), static_cast<std::size_t> (sizeInBytes)));
        if (state.contains ("settings"))
            m_state.settings () = state.at ("settings").get<algo::GenerationSettings> ();
        m_preview = state.value ("preview", true);
        if (state.contains ("starter"))
            m_state.setStarter (state.at ("starter").get<algo::SongStarter> ());
    }
    catch (const std::exception& error)
    {
        m_state.setStatus ("The saved plugin state is not valid: " + juce::String (error.what ()));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter ()
{
    return new PluginProcessor ();
}

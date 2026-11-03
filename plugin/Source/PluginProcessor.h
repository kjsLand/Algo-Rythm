#pragma once

#include "AppState.h"
#include "Sequencer.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

// The Algo-Rythm plugin (T16-01). It follows the host tempo and position (REQ-16-2) and sends the song starter
// as MIDI. The song starter repeats for as long as the host plays. "Preview Sound" plays it with the
// plucked-string synthesizer of the app.
class PluginProcessor : public juce::AudioProcessor,
                        private juce::ChangeListener
{
public:
    PluginProcessor ();
    ~PluginProcessor () override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources () override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor () override;
    bool                        hasEditor () const override
    {
        return true;
    }

    const juce::String getName () const override
    {
        return JucePlugin_Name;
    }
    bool acceptsMidi () const override
    {
        return false;
    }
    bool producesMidi () const override
    {
        return true;
    }
    bool isMidiEffect () const override
    {
        return false;
    }
    double getTailLengthSeconds () const override
    {
        return 2.0;
    }

    int getNumPrograms () override
    {
        return 1;
    }
    int getCurrentProgram () override
    {
        return 0;
    }
    void               setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override
    {
        return "Default";
    }
    void changeProgramName (int, const juce::String&) override {}

    // Saves the current song starter as JSON, so a reopened DAW project restores it.
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    AppState& state ()
    {
        return m_state;
    }

    void setPreviewSound (bool on)
    {
        m_preview = on;
    }
    bool previewSound () const
    {
        return m_preview.load ();
    }

    // The last host position in ticks, or -1 when the host stops. The song overview draws it.
    double positionTicks () const
    {
        return m_shownTicks.load ();
    }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void allNotesOff (juce::MidiBuffer& midi, int offset);

    AppState          m_state;
    juce::Synthesiser m_synth;
    ScheduleSlot      m_slot;
    EventSchedule*    m_current { nullptr };
    EventSchedule*    m_waiting { nullptr }; // Audio thread only

    SoundingNotes       m_sounding {};
    bool                m_wasPlaying { false };
    double              m_lastEndTick { -1.0 };
    double              m_sampleRate { 48000.0 };
    std::atomic<bool>   m_preview { true };
    std::atomic<double> m_shownTicks { -1.0 };

    std::shared_ptr<const algo::SongStarter> m_scheduled;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};

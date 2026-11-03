#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <vector>

// The sounds of the app synthesizer. Channel 10 plays the click and the chuck. The other channels pluck strings.
class PluckSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override
    {
        return true;
    }
    bool appliesToChannel (int channel) override
    {
        return channel != 10;
    }
};

class ClickSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override
    {
        return true;
    }
    bool appliesToChannel (int channel) override
    {
        return channel == 10;
    }
};

// A plucked string with the Karplus-Strong method (T12-02). The delay line has noise from a fixed seed,
// so a note sounds the same on each play. Only prepare allocates memory.
class PluckVoice : public juce::SynthesiserVoice
{
public:
    static constexpr double kLowestFrequency = 20.0;
    static constexpr double kE2Frequency     = 82.4069;
    static constexpr double kE2DecaySeconds  = 4.0; // 60 dB of decay
    static constexpr int    kNoiseSeed       = 0x5eed;

    // Allocates the delay line. Call this before the first note, and when the sample rate changes.
    void prepare (double sampleRate);

    // The time for a decay of 60 dB at a frequency. E2 takes 4 seconds. A higher note decays faster.
    static double decaySeconds (double frequency);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override;
    using juce::SynthesiserVoice::renderNextBlock;

private:
    std::vector<float> m_line;
    double             m_sampleRate { 0.0 };
    int                m_write { 0 };
    double             m_delay { 0.0 }; // The read distance in samples
    float              m_previous { 0.0f };
    float              m_feedback { 0.99f };
    float              m_level { 0.0f };
    float              m_release { 1.0f };
    float              m_releaseStep { 1.0f };
    bool               m_releasing { false };
    int                m_quietSamples { 0 };
};

// A short click for the metronome and a noise burst for a chuck.
class ClickVoice : public juce::SynthesiserVoice
{
public:
    void prepare (double sampleRate);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override;
    void stopNote (float, bool) override {}
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override;
    using juce::SynthesiserVoice::renderNextBlock;

private:
    std::vector<float> m_noise;
    double             m_sampleRate { 0.0 };
    double             m_phase { 0.0 };
    double             m_step { 0.0 };
    float              m_level { 0.0f };
    float              m_decay { 0.999f };
    bool               m_noiseNote { false };
    int                m_position { 0 };
};

// Adds the voices and the sounds to a synthesizer.
void configureSynth (juce::Synthesiser& synth, int pluckVoices = 32, int clickVoices = 4);

// Sets the sample rate of the synthesizer and prepares each voice.
void prepareSynth (juce::Synthesiser& synth, double sampleRate);

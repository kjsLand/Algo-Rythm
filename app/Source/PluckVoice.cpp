#include "PluckVoice.h"

#include <cmath>

void PluckVoice::prepare (double sampleRate)
{
    m_sampleRate = sampleRate;
    m_line.assign (static_cast<std::size_t> (std::ceil (sampleRate / kLowestFrequency)) + 4, 0.0f);
    m_write = 0;
}

double PluckVoice::decaySeconds (double frequency)
{
    return std::max (0.8, kE2DecaySeconds * std::sqrt (kE2Frequency / frequency));
}

bool PluckVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<PluckSound*> (sound) != nullptr;
}

void PluckVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    if (m_line.empty () || m_sampleRate <= 0.0)
    {
        clearCurrentNote ();
        return;
    }

    const double frequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    const int    size      = static_cast<int> (m_line.size ());

    // The averaging filter adds half a sample of delay to the loop.
    m_delay = juce::jlimit (2.0, static_cast<double> (size - 2), m_sampleRate / frequency - 0.5);

    // Each pass round the loop multiplies the level by the feedback.
    const double passes = decaySeconds (frequency) * frequency;
    m_feedback          = static_cast<float> (std::pow (10.0, -3.0 / passes));

    // Fill the loop with noise from a fixed seed, with no DC offset.
    juce::Random random (kNoiseSeed);
    const int    filled = static_cast<int> (std::ceil (m_delay)) + 1;
    std::fill (m_line.begin (), m_line.end (), 0.0f);
    double sum = 0.0;
    for (int i = 0; i < filled; ++i)
    {
        const float value                    = random.nextFloat () * 2.0f - 1.0f;
        m_line[static_cast<std::size_t> (i)] = value;
        sum += value;
    }
    const float mean = static_cast<float> (sum / filled);
    for (int i = 0; i < filled; ++i)
        m_line[static_cast<std::size_t> (i)] -= mean;

    m_write        = filled % size;
    m_previous     = 0.0f;
    m_level        = 0.35f * velocity;
    m_release      = 1.0f;
    m_releasing    = false;
    m_quietSamples = 0;
}

void PluckVoice::stopNote (float, bool allowTailOff)
{
    if (!allowTailOff)
    {
        clearCurrentNote ();
        return;
    }
    // A finger on the string damps it in about 80 ms.
    m_releasing   = true;
    m_releaseStep = static_cast<float> (std::pow (0.001, 1.0 / (0.08 * m_sampleRate)));
}

void PluckVoice::renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples)
{
    if (!isVoiceActive ())
        return;

    const int size = static_cast<int> (m_line.size ());
    for (int n = 0; n < numSamples; ++n)
    {
        // Read the loop at a fractional distance with linear interpolation.
        double read = m_write - m_delay;
        if (read < 0.0)
            read += size;
        const int   i0   = static_cast<int> (read);
        const int   i1   = (i0 + 1) % size;
        const float frac = static_cast<float> (read - i0);
        const float y    = m_line[static_cast<std::size_t> (i0)] + frac * (m_line[static_cast<std::size_t> (i1)] - m_line[static_cast<std::size_t> (i0)]);

        // The two-point average filter in the feedback loop.
        const float filtered                       = m_feedback * 0.5f * (y + m_previous);
        m_previous                                 = y;
        m_line[static_cast<std::size_t> (m_write)] = filtered;
        m_write                                    = (m_write + 1) % size;

        if (m_releasing)
            m_release *= m_releaseStep;

        const float sample = filtered * m_level * m_release;
        for (int channel = 0; channel < output.getNumChannels (); ++channel)
            output.addSample (channel, startSample + n, sample);

        m_quietSamples = std::abs (sample) < 1.0e-5f ? m_quietSamples + 1 : 0;
        if (m_quietSamples > 512 || m_release < 0.001f)
        {
            clearCurrentNote ();
            return;
        }
    }
}

void ClickVoice::prepare (double sampleRate)
{
    m_sampleRate = sampleRate;
    m_noise.assign (static_cast<std::size_t> (sampleRate * 0.06), 0.0f);
    juce::Random random (PluckVoice::kNoiseSeed + 1);
    for (auto& value : m_noise)
        value = random.nextFloat () * 2.0f - 1.0f;
}

bool ClickVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<ClickSound*> (sound) != nullptr;
}

void ClickVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    if (m_sampleRate <= 0.0)
    {
        clearCurrentNote ();
        return;
    }
    // Pitch 76 is beat 1, pitch 77 the other beats and pitch 39 a chuck (hand clap).
    m_noiseNote = midiNoteNumber == 39;
    m_phase     = 0.0;
    m_step      = juce::MathConstants<double>::twoPi * (midiNoteNumber == 76 ? 1760.0 : 1320.0) / m_sampleRate;
    m_level     = (m_noiseNote ? 0.5f : 0.3f) * velocity;
    m_decay     = static_cast<float> (std::pow (0.001, 1.0 / ((m_noiseNote ? 0.05 : 0.03) * m_sampleRate)));
    m_position  = 0;
}

void ClickVoice::renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples)
{
    if (!isVoiceActive ())
        return;

    for (int n = 0; n < numSamples; ++n)
    {
        float sample = 0.0f;
        if (m_noiseNote)
            sample = m_position < static_cast<int> (m_noise.size ()) ? m_noise[static_cast<std::size_t> (m_position)] : 0.0f;
        else
            sample = static_cast<float> (std::sin (m_phase));

        m_phase += m_step;
        ++m_position;
        sample *= m_level;
        m_level *= m_decay;

        for (int channel = 0; channel < output.getNumChannels (); ++channel)
            output.addSample (channel, startSample + n, sample);

        if (m_level < 1.0e-4f)
        {
            clearCurrentNote ();
            return;
        }
    }
}

void configureSynth (juce::Synthesiser& synth, int pluckVoices, int clickVoices)
{
    synth.clearVoices ();
    synth.clearSounds ();
    for (int i = 0; i < pluckVoices; ++i)
        synth.addVoice (new PluckVoice ());
    for (int i = 0; i < clickVoices; ++i)
        synth.addVoice (new ClickVoice ());
    synth.addSound (new PluckSound ());
    synth.addSound (new ClickSound ());

    // Events start within 16 samples of their time in the block.
    synth.setMinimumRenderingSubdivisionSize (16, false);
}

void prepareSynth (juce::Synthesiser& synth, double sampleRate)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    for (int i = 0; i < synth.getNumVoices (); ++i)
    {
        if (auto* pluck = dynamic_cast<PluckVoice*> (synth.getVoice (i)))
            pluck->prepare (sampleRate);
        else if (auto* click = dynamic_cast<ClickVoice*> (synth.getVoice (i)))
            click->prepare (sampleRate);
    }
}

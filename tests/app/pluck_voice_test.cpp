#include "AllocationCounter.h"

#include "PluckVoice.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

namespace
{
    constexpr double kSampleRate = 48000.0;

    std::vector<float> renderNote (int note, double seconds)
    {
        juce::Synthesiser synth;
        configureSynth (synth, 1, 0);
        prepareSynth (synth, kSampleRate);

        const int                samples = static_cast<int> (seconds * kSampleRate);
        juce::AudioBuffer<float> buffer (1, samples);
        buffer.clear ();
        synth.noteOn (1, note, 1.0f);
        juce::MidiBuffer midi;
        synth.renderNextBlock (buffer, midi, 0, samples);
        return std::vector<float> (buffer.getReadPointer (0), buffer.getReadPointer (0) + samples);
    }

    // The frequency from the autocorrelation peak, with parabolic interpolation.
    double estimateFrequency (const std::vector<float>& x, int from, int length, double lo, double hi)
    {
        const int           minLag = static_cast<int> (kSampleRate / hi);
        const int           maxLag = static_cast<int> (kSampleRate / lo) + 1;
        std::vector<double> r (static_cast<std::size_t> (maxLag + 2), 0.0);
        for (int lag = minLag - 1; lag <= maxLag + 1; ++lag)
            for (int i = from; i < from + length; ++i)
                r[static_cast<std::size_t> (lag)] += static_cast<double> (x[static_cast<std::size_t> (i)]) * x[static_cast<std::size_t> (i + lag)];

        int best = minLag;
        for (int lag = minLag; lag <= maxLag; ++lag)
            if (r[static_cast<std::size_t> (lag)] > r[static_cast<std::size_t> (best)])
                best = lag;

        const double a = r[static_cast<std::size_t> (best - 1)], b = r[static_cast<std::size_t> (best)], c = r[static_cast<std::size_t> (best + 1)];
        const double shift = 0.5 * (a - c) / (a - 2.0 * b + c);
        return kSampleRate / (best + shift);
    }

    double rms (const std::vector<float>& x, int from, int length)
    {
        double sum = 0.0;
        for (int i = from; i < from + length; ++i)
            sum += static_cast<double> (x[static_cast<std::size_t> (i)]) * x[static_cast<std::size_t> (i)];
        return std::sqrt (sum / length);
    }
}

TEST_CASE ("An offline A4 has a frequency of 440 Hz within 2 Hz")
{
    const auto x = renderNote (69, 0.5);
    REQUIRE (std::abs (estimateFrequency (x, 2400, 9600, 300.0, 700.0) - 440.0) <= 2.0);
}

TEST_CASE ("An offline E2 has a frequency of 82.4 Hz and decays by about 60 dB in 4 seconds")
{
    const auto x = renderNote (40, 4.2);
    REQUIRE (std::abs (estimateFrequency (x, 4800, 24000, 60.0, 120.0) - 82.41) <= 1.0);

    const double start = rms (x, 2400, 4800);
    const double end   = rms (x, static_cast<int> (3.95 * kSampleRate), 4800);
    const double db    = 20.0 * std::log10 (end / start);
    REQUIRE (db < -50.0);
    REQUIRE (db > -72.0);
}

TEST_CASE ("The same note sounds the same on each play")
{
    REQUIRE (renderNote (52, 0.2) == renderNote (52, 0.2));
}

TEST_CASE ("The voice makes no allocation in renderNextBlock")
{
    juce::Synthesiser synth;
    configureSynth (synth, 4, 1);
    prepareSynth (synth, kSampleRate);
    juce::AudioBuffer<float> buffer (2, 256);
    juce::MidiBuffer         midi;
    midi.ensureSize (1024);

    long allocations = 0;
    {
        algo::test::ScopedAllocationCount count;
        for (int block = 0; block < 400; ++block)
        {
            midi.clear ();
            if (block % 20 == 0)
            {
                midi.addEvent (juce::MidiMessage::noteOn (1, 40 + block % 24, 0.8f), 10);
                midi.addEvent (juce::MidiMessage::noteOn (10, 76, 0.8f), 20);
            }
            if (block % 20 == 10)
                midi.addEvent (juce::MidiMessage::noteOff (1, 40 + (block - 10) % 24), 5);
            synth.renderNextBlock (buffer, midi, 0, 256);
        }
        allocations = count.count ();
    }
    REQUIRE (allocations == 0);
}

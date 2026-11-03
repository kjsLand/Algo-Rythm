#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

// Shows the audio device selector in a dialog window (T12-05).
void showAudioSettingsDialog (juce::AudioDeviceManager& devices);

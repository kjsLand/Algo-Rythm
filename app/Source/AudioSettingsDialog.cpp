#include "AudioSettingsDialog.h"

#include "Theme.h"

#include <juce_audio_utils/juce_audio_utils.h>

void showAudioSettingsDialog (juce::AudioDeviceManager& devices)
{
    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent> (devices, 0, 0, 2, 2, false, false, true, false);
    selector->setSize (520, 360);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (selector.release ());
    options.dialogTitle                  = "Audio Settings";
    options.dialogBackgroundColour       = Theme::background;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar            = true;
    options.resizable                    = true;
    options.launchAsync ();
}

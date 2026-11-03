#include "AppState.h"
#include "AudioEngine.h"
#include "AudioSettingsDialog.h"
#include "MainComponent.h"
#include "Preferences.h"
#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

class AlgoRythmApplication : public juce::JUCEApplication,
                             private juce::ChangeListener
{
public:
    const juce::String getApplicationName () override
    {
        return JUCE_APPLICATION_NAME_STRING;
    }
    const juce::String getApplicationVersion () override
    {
        return JUCE_APPLICATION_VERSION_STRING;
    }
    bool moreThanOneInstanceAllowed () override
    {
        return true;
    }

    void initialise (const juce::String&) override
    {
        auto& laf = juce::LookAndFeel::getDefaultLookAndFeel ();
        laf.setColour (juce::ResizableWindow::backgroundColourId, Theme::background);

        m_state              = std::make_unique<AppState> ();
        m_state->settings () = m_prefs.settings ();
        m_state->setAbTestMode (m_prefs.abTestMode ());
        m_state->setLeftHanded (m_prefs.leftHanded ());
        if (m_state->settings ().tables == "learned")
            m_state->setTables ("learned");

        const auto audio = m_prefs.audioState ();
        m_audio          = std::make_unique<AudioEngine> (audio.get ());
        m_audio->deviceManager ().addChangeListener (this);
        if (m_audio->errorText ().isNotEmpty ())
            m_state->setStatus ("Audio: " + m_audio->errorText ());

        m_window = std::make_unique<MainWindow> (getApplicationName (), *this);
        m_state->requestGenerate ();
    }

    void shutdown () override
    {
        if (m_window != nullptr)
        {
            m_prefs.setWindowBounds (m_window->getWindowStateAsString ());
            m_window = nullptr;
        }
        if (m_audio != nullptr)
        {
            m_audio->deviceManager ().removeChangeListener (this);
            const auto xml = m_audio->deviceManager ().createStateXml ();
            m_prefs.setAudioState (xml.get ());
        }
        if (m_state != nullptr)
        {
            m_prefs.setSettings (m_state->settings ());
            m_prefs.setAbTestMode (m_state->abTestMode ());
            m_prefs.setLeftHanded (m_state->leftHanded ());
        }
        m_prefs.save ();
        m_audio = nullptr;
        m_state = nullptr;
    }

    void systemRequestedQuit () override
    {
        quit ();
    }
    void anotherInstanceStarted (const juce::String&) override {}

private:
    // The device selection is saved when it changes (T12-05).
    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        const auto xml = m_audio->deviceManager ().createStateXml ();
        m_prefs.setAudioState (xml.get ());
        m_prefs.save ();
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (const juce::String& name, AlgoRythmApplication& app)
            : DocumentWindow (name, Theme::background, DocumentWindow::allButtons)
        {
            auto* content            = new MainComponent (*app.m_state, &app.m_audio->sequencer ());
            content->onAudioSettings = [&app]
            {
                showAudioSettingsDialog (app.m_audio->deviceManager ());
            };
            content->onQuit = []
            {
                JUCEApplication::getInstance ()->systemRequestedQuit ();
            };
            content->audioError = [&app]
            {
                return app.m_audio->errorText ();
            };
            content->onClickChanged = [&app] (bool on)
            {
                app.m_prefs.setClickOn (on);
            };
            content->onExportFolderChanged = [&app] (juce::File folder)
            {
                app.m_prefs.setLastExportFolder (folder);
            };
            content->setExportFolder (app.m_prefs.lastExportFolder ());
            content->setClick (app.m_prefs.clickOn ());

#if JUCE_MAC
            juce::MenuBarModel::setMacMainMenu (content);
#else
            content->showMenuBar (true);
#endif
            setUsingNativeTitleBar (true);
            setContentOwned (content, true);
            setResizable (true, true);
            setResizeLimits (MainComponent::kMinWidth, MainComponent::kMinHeight, 10000, 10000);

            const auto saved = app.m_prefs.windowBounds ();
            if (saved.isEmpty () || !restoreWindowStateFromString (saved))
                centreWithSize (1200, 800);
            setVisible (true);
        }

        ~MainWindow () override
        {
#if JUCE_MAC
            juce::MenuBarModel::setMacMainMenu (nullptr);
#endif
        }

        void closeButtonPressed () override
        {
            JUCEApplication::getInstance ()->systemRequestedQuit ();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    Preferences                  m_prefs;
    std::unique_ptr<AppState>    m_state;
    std::unique_ptr<AudioEngine> m_audio;
    std::unique_ptr<MainWindow>  m_window;
};

START_JUCE_APPLICATION (AlgoRythmApplication)

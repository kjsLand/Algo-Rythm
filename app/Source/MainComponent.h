#pragma once

#include "AppState.h"
#include "ControlsPanel.h"
#include "ExportPanel.h"
#include "FretboardView.h"
#include "LibraryPanel.h"
#include "Sequencer.h"
#include "SongView.h"
#include "StrumView.h"
#include "TransportBar.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

// The main window content: controls, song overview, fretboard, strum panel, transport, export and library.
// The app and the plugin both use it (REQ-16-1). With no sequencer, the transport bar is hidden.
class MainComponent : public juce::Component,
                      public juce::MenuBarModel,
                      private juce::KeyListener,
                      private juce::ChangeListener
{
public:
    static constexpr int kMinWidth  = 900;
    static constexpr int kMinHeight = 600;

    MainComponent (AppState& state, Sequencer* sequencer);
    ~MainComponent () override;

    // The app sets these. The plugin leaves them empty.
    std::function<void ()>           onAudioSettings;
    std::function<void ()>           onQuit;
    std::function<juce::String ()>   audioError;
    std::function<void (bool)>       onClickChanged;
    std::function<void (juce::File)> onExportFolderChanged;

    void setExportFolder (const juce::File& folder);
    void setClick (bool on);
    void showMenuBar (bool show);

    // Runs a keyboard shortcut. Returns false if the key has no shortcut.
    bool handleShortcut (const juce::KeyPress& key);

    void paint (juce::Graphics& g) override;
    void resized () override;

    // juce::MenuBarModel
    juce::StringArray getMenuBarNames () override;
    juce::PopupMenu   getMenuForIndex (int index, const juce::String& name) override;
    void              menuItemSelected (int itemId, int index) override;

private:
    bool       keyPressed (const juce::KeyPress& key, juce::Component* origin) override;
    bool       keyPressed (const juce::KeyPress& key) override;
    void       changeListenerCallback (juce::ChangeBroadcaster*) override;
    void       attachKeys (juce::Component& component);
    juce::File writeSectionMidi (int section);

    AppState&  m_state;
    Sequencer* m_sequencer;

    ControlsPanel                           m_controls;
    SongView                                m_song;
    FretboardView                           m_fretboard;
    StrumView                               m_strum;
    ExportPanel                             m_export;
    LibraryPanel                            m_library;
    std::unique_ptr<TransportBar>           m_transport;
    std::unique_ptr<juce::MenuBarComponent> m_menuBar;
    juce::TooltipWindow                     m_tooltips { this, 600 };

    std::shared_ptr<const algo::SongStarter> m_scheduled;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

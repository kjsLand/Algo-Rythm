#pragma once

#include "AppState.h"
#include "RatingControl.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

// The export buttons (T11-08), the Save button (T14-04) and the rating stars (T14-05).
class ExportPanel : public juce::Component,
                    private juce::ChangeListener
{
public:
    explicit ExportPanel (AppState& state);
    ~ExportPanel () override;

    // The folder of the last export. Preferences save it.
    juce::File                       lastFolder;
    std::function<void (juce::File)> onFolderChanged;
    std::function<void ()>           onSaved;

    // "mid", "tab", "json" or "musicxml"
    void exportAs (const juce::String& format);
    void saveToLibrary ();

    void paint (juce::Graphics& g) override;
    void resized () override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    AppState&                          m_state;
    juce::TextButton                   m_midi { "Export MIDI" }, m_tab { "Export Tab" }, m_json { "Export JSON" }, m_xml { "MusicXML" }, m_save { "Save" };
    RatingControl                      m_rating;
    std::unique_ptr<juce::FileChooser> m_chooser;
};

#pragma once

#include "AppState.h"
#include "Library.h"

#include <juce_gui_basics/juce_gui_basics.h>

// The side panel with the saved song starters (T14-04). A double-click loads a file into AppState.
class LibraryPanel : public juce::Component,
                     private juce::ListBoxModel
{
public:
    explicit LibraryPanel (AppState& state);

    void refresh ();

    void paint (juce::Graphics& g) override;
    void resized () override;

private:
    int  getNumRows () override;
    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;
    void returnKeyPressed (int row) override;
    void deleteKeyPressed (int row) override;

    void loadRow (int row);
    void deleteRow (int row);

    AppState&                   m_state;
    Library                     m_library;
    std::vector<Library::Entry> m_entries;
    juce::ListBox               m_list { "Library", this };
    juce::TextButton            m_delete { "Delete" }, m_refresh { "Refresh" };
};

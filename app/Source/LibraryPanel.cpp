#include "LibraryPanel.h"

#include "Theme.h"

LibraryPanel::LibraryPanel (AppState& state)
    : m_state (state)
{
    m_list.setRowHeight (40);
    m_list.setColour (juce::ListBox::backgroundColourId, Theme::panel);
    m_list.setTitle ("Saved song starters");
    addAndMakeVisible (m_list);

    m_delete.setTooltip ("Move the selected file to the trash");
    m_delete.onClick = [this]
    {
        deleteRow (m_list.getSelectedRow ());
    };
    m_refresh.onClick = [this]
    {
        refresh ();
    };
    addAndMakeVisible (m_delete);
    addAndMakeVisible (m_refresh);
    refresh ();
}

void LibraryPanel::refresh ()
{
    m_entries = m_library.list ();
    m_list.updateContent ();
    m_list.repaint ();
}

int LibraryPanel::getNumRows ()
{
    return static_cast<int> (m_entries.size ());
}

void LibraryPanel::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (row < 0 || row >= getNumRows ())
        return;
    const auto& entry = m_entries[static_cast<std::size_t> (row)];
    if (selected)
    {
        g.setColour (Theme::panelRaised);
        g.fillRect (0, 0, width, height);
    }
    g.setColour (Theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (entry.key + "  " + juce::String (entry.bpm) + " BPM", 6, 2, width - 12, 18, juce::Justification::centredLeft, true);
    g.setColour (Theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    juce::String detail = entry.date.formatted ("%Y-%m-%d %H:%M");
    if (entry.rating > 0)
        detail << "   " << juce::String::repeatedString ("*", entry.rating);
    g.drawText (detail, 6, 20, width - 12, 16, juce::Justification::centredLeft, true);
}

void LibraryPanel::loadRow (int row)
{
    if (row < 0 || row >= getNumRows ())
        return;
    try
    {
        m_state.setStarter (m_library.load (m_entries[static_cast<std::size_t> (row)].file));
        m_state.setStatus ("Loaded " + m_entries[static_cast<std::size_t> (row)].file.getFileName ());
    }
    catch (const std::exception& error)
    {
        m_state.setStatus ("Error: " + juce::String (error.what ()));
    }
}

void LibraryPanel::deleteRow (int row)
{
    if (row < 0 || row >= getNumRows ())
        return;
    const auto file = m_entries[static_cast<std::size_t> (row)].file;
    if (!m_library.remove (file))
        m_state.setStatus ("Error: " + file.getFileName () + " cannot move to the trash");
    refresh ();
}

void LibraryPanel::listBoxItemDoubleClicked (int row, const juce::MouseEvent&)
{
    loadRow (row);
}

void LibraryPanel::returnKeyPressed (int row)
{
    loadRow (row);
}

void LibraryPanel::deleteKeyPressed (int row)
{
    deleteRow (row);
}

void LibraryPanel::paint (juce::Graphics& g)
{
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (getLocalBounds ().toFloat (), Theme::radius);
    g.setColour (Theme::text);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText ("Library", getLocalBounds ().reduced (Theme::gap).removeFromTop (20), juce::Justification::centredLeft);
}

void LibraryPanel::resized ()
{
    auto r = getLocalBounds ().reduced (Theme::gap);
    r.removeFromTop (24);
    auto buttons = r.removeFromBottom (28);
    m_delete.setBounds (buttons.removeFromLeft (buttons.getWidth () / 2 - 2));
    buttons.removeFromLeft (4);
    m_refresh.setBounds (buttons);
    r.removeFromBottom (4);
    m_list.setBounds (r);
}

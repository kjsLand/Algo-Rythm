#include "ExportPanel.h"

#include "Library.h"
#include "Theme.h"

#include "algo/Files.h"

ExportPanel::ExportPanel (AppState& state)
    : m_state (state)
{
    m_midi.setTooltip ("Write a Standard MIDI File (E)");
    m_tab.setTooltip ("Write the ASCII tab as a text file");
    m_json.setTooltip ("Write the song starter as JSON");
    m_xml.setTooltip ("Write a MusicXML score with notation and tab");
    m_save.setTooltip ("Save to the library (S)");

    m_midi.onClick = [this]
    {
        exportAs ("mid");
    };
    m_tab.onClick = [this]
    {
        exportAs ("tab");
    };
    m_json.onClick = [this]
    {
        exportAs ("json");
    };
    m_xml.onClick = [this]
    {
        exportAs ("musicxml");
    };
    m_save.onClick = [this]
    {
        saveToLibrary ();
    };
    m_rating.onRate = [this] (int rating)
    {
        m_state.rate (rating);
        if (onSaved)
            onSaved ();
    };

    for (auto* c : std::initializer_list<juce::Component*> { &m_midi, &m_tab, &m_json, &m_xml, &m_save, &m_rating })
    {
        c->setWantsKeyboardFocus (true);
        addAndMakeVisible (c);
    }
    lastFolder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    m_state.addChangeListener (this);
}

ExportPanel::~ExportPanel ()
{
    m_state.removeChangeListener (this);
}

void ExportPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    const auto starter = m_state.starter ();
    m_rating.setRating (starter != nullptr ? starter->rating.value_or (0) : 0);
    for (auto* c : std::initializer_list<juce::Component*> { &m_midi, &m_tab, &m_json, &m_xml, &m_save, &m_rating })
        c->setEnabled (starter != nullptr);
}

void ExportPanel::exportAs (const juce::String& format)
{
    const auto starter = m_state.starter ();
    if (starter == nullptr)
        return;

    const auto extension = juce::String (algo::formatExtension (format.toStdString ()));
    const auto initial   = lastFolder.getChildFile (juce::String (algo::defaultFileName (*starter)) + extension);
    m_chooser            = std::make_unique<juce::FileChooser> ("Export " + format, initial, "*" + extension);

    juce::Component::SafePointer<ExportPanel> safe (this);
    m_chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                            [safe, starter, extension] (const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult ();
        if (safe == nullptr || file == juce::File ())
            return;
        file = file.withFileExtension (extension);
        try
        {
            algo::exportFile (*starter, file.getFullPathName ().toStdString (), safe->m_state.data ());
            safe->lastFolder = file.getParentDirectory ();
            if (safe->onFolderChanged)
                safe->onFolderChanged (safe->lastFolder);
            safe->m_state.setStatus ("Exported " + file.getFullPathName ());
        }
        catch (const std::exception& error)
        {
            safe->m_state.setStatus ("Error: " + juce::String (error.what ()));
        }
    });
}

void ExportPanel::saveToLibrary ()
{
    const auto starter = m_state.starter ();
    if (starter == nullptr)
        return;
    try
    {
        const auto file = Library ().save (*starter);
        m_state.setStatus ("Saved " + file.getFileName ());
        if (onSaved)
            onSaved ();
    }
    catch (const std::exception& error)
    {
        m_state.setStatus ("Error: " + juce::String (error.what ()));
    }
}

void ExportPanel::paint (juce::Graphics& g)
{
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (getLocalBounds ().toFloat (), Theme::radius);
}

void ExportPanel::resized ()
{
    auto r     = getLocalBounds ().reduced (Theme::gap);
    auto top   = r.removeFromTop (r.getHeight () / 2).reduced (0, 2);
    auto below = r.reduced (0, 2);

    const int  w      = (top.getWidth () - 3 * Theme::gap) / 4;
    const bool narrow = w < 96;
    m_midi.setButtonText (narrow ? "MIDI" : "Export MIDI");
    m_tab.setButtonText (narrow ? "Tab" : "Export Tab");
    m_json.setButtonText (narrow ? "JSON" : "Export JSON");
    for (auto* b : { &m_midi, &m_tab, &m_json, &m_xml })
    {
        b->setBounds (top.removeFromLeft (w));
        top.removeFromLeft (Theme::gap);
    }
    m_save.setBounds (below.removeFromLeft (w));
    below.removeFromLeft (Theme::gap);
    m_rating.setBounds (below.removeFromLeft (std::min (below.getWidth (), 150)));
}

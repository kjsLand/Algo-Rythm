#include "MainComponent.h"

#include "Theme.h"

#include "algo/Files.h"
#include "algo/Render.h"
#include "algo/Smf.h"

namespace
{
    enum MenuItem
    {
        kGenerate = 1,
        kSave,
        kExportMidi,
        kExportTab,
        kExportJson,
        kExportXml,
        kQuit,
        kAudioSettings,
        kTablesRules,
        kTablesLearned,
        kAbTest,
        kLeftHanded,
        kShortcuts
    };

    const char* kShortcutText = "G: Generate\n1 to 4: Select section 1 to 4\nSpace: Play and Stop\nS: Save to the library\nE: Export MIDI\nLeft and Right: Select a bar in the song overview";
}

MainComponent::MainComponent (AppState& state, Sequencer* sequencer)
    : m_state (state),
      m_sequencer (sequencer),
      m_controls (state),
      m_song (state),
      m_fretboard (state),
      m_strum (state),
      m_export (state),
      m_library (state)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

    for (auto* c : std::initializer_list<juce::Component*> { &m_controls, &m_song, &m_fretboard, &m_strum, &m_export, &m_library })
        addAndMakeVisible (c);

    if (m_sequencer != nullptr)
    {
        m_transport                 = std::make_unique<TransportBar> (state, *m_sequencer);
        m_transport->onClickChanged = [this] (bool on)
        {
            if (onClickChanged)
                onClickChanged (on);
        };
        addAndMakeVisible (*m_transport);
        m_song.setPositionProvider ([this]
        {
            return m_sequencer->positionTicks ();
        });
    }

    m_song.makeSectionMidi = [this] (int section)
    {
        return writeSectionMidi (section);
    };
    m_export.onFolderChanged = [this] (juce::File folder)
    {
        if (onExportFolderChanged)
            onExportFolderChanged (folder);
    };
    m_export.onSaved = [this]
    {
        m_library.refresh ();
    };

    m_state.addChangeListener (this);
    attachKeys (*this);
    setSize (1200, 800);
}

MainComponent::~MainComponent ()
{
    m_state.removeChangeListener (this);
    if (m_menuBar != nullptr)
        m_menuBar->setModel (nullptr);
}

void MainComponent::showMenuBar (bool show)
{
    if (show && m_menuBar == nullptr)
    {
        m_menuBar = std::make_unique<juce::MenuBarComponent> (this);
        addAndMakeVisible (*m_menuBar);
    }
    else if (!show)
        m_menuBar.reset ();
    resized ();
}

void MainComponent::setExportFolder (const juce::File& folder)
{
    m_export.lastFolder = folder;
}

void MainComponent::setClick (bool on)
{
    if (m_transport != nullptr)
        m_transport->setClick (on);
}

void MainComponent::attachKeys (juce::Component& component)
{
    // A text field needs its keys. The other components pass the shortcut keys to this component.
    if (dynamic_cast<juce::TextEditor*> (&component) == nullptr && &component != this)
        component.addKeyListener (this);
    for (auto* child : component.getChildren ())
        attachKeys (*child);
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    attachKeys (*this);
    menuItemsChanged ();

    const auto starter = m_state.starter ();
    if (starter != m_scheduled && starter != nullptr && m_sequencer != nullptr)
    {
        m_sequencer->setSchedule (EventSchedule::fromStarter (*starter, m_state.data ()));
        m_scheduled = starter;
    }
    repaint (getLocalBounds ().removeFromBottom (24));
}

bool MainComponent::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    return handleShortcut (key);
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    return handleShortcut (key);
}

bool MainComponent::handleShortcut (const juce::KeyPress& key)
{
    if (key.getModifiers ().isCommandDown () || key.getModifiers ().isAltDown ())
        return false;

    const auto c = juce::CharacterFunctions::toLowerCase (key.getTextCharacter ());
    if (c == 'g')
    {
        m_state.requestGenerate ();
        return true;
    }
    if (c >= '1' && c <= '4')
    {
        const auto starter = m_state.starter ();
        const int  section = static_cast<int> (c - '1');
        if (starter != nullptr && section < static_cast<int> (starter->sections.size ()))
            m_state.select (section, 0);
        return true;
    }
    if (key == juce::KeyPress::spaceKey)
    {
        if (m_transport != nullptr)
            m_transport->togglePlay ();
        return true;
    }
    if (c == 's')
    {
        m_export.saveToLibrary ();
        return true;
    }
    if (c == 'e')
    {
        m_export.exportAs ("mid");
        return true;
    }
    return false;
}

juce::File MainComponent::writeSectionMidi (int section)
{
    const auto starter = m_state.starter ();
    if (starter == nullptr || section < 0 || section >= static_cast<int> (starter->sections.size ()))
        return {};

    const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Algo-Rythm");
    folder.createDirectory ();
    const auto file = folder.getChildFile (juce::String (algo::defaultFileName (*starter)) + "-" + juce::String (starter->sections[static_cast<std::size_t> (section)].name).removeCharacters (" ") + ".mid");
    try
    {
        // The clip has no click, so it has the length of the section only.
        algo::writeSmfFile (algo::render (*starter, algo::RenderOptions { false, true, section }, m_state.data ()), file.getFullPathName ().toStdString ());
    }
    catch (const std::exception& error)
    {
        m_state.setStatus ("Error: " + juce::String (error.what ()));
        return {};
    }
    return file;
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);

    auto status = getLocalBounds ().removeFromBottom (24).reduced (Theme::gap, 2);
    g.setFont (juce::FontOptions (13.0f));
    const auto error = audioError ? audioError () : juce::String ();
    if (error.isNotEmpty ())
    {
        g.setColour (Theme::warning);
        const auto text = "Audio: " + error;
        g.drawText (text, status.removeFromRight (std::min (status.getWidth () / 2, 420)), juce::Justification::centredRight, true);
    }
    g.setColour (m_state.status ().startsWith ("Error") ? Theme::warning : Theme::textDim);
    g.drawText (m_state.status (), status, juce::Justification::centredLeft, true);
}

void MainComponent::resized ()
{
    auto r = getLocalBounds ();
    if (m_menuBar != nullptr)
        m_menuBar->setBounds (r.removeFromTop (24));
    r.removeFromBottom (24); // The status bar
    r.reduce (Theme::gap, Theme::gap);

    m_library.setBounds (r.removeFromRight (juce::jlimit (160, 260, r.getWidth () / 5)));
    r.removeFromRight (Theme::gap);

    m_controls.setBounds (r.removeFromTop (106));
    r.removeFromTop (Theme::gap);

    auto bottom = r.removeFromBottom (r.getHeight () < 520 ? 112 : 124);
    r.removeFromBottom (Theme::gap);

    const int fretboardHeight = juce::jlimit (100, 230, static_cast<int> (r.getHeight () * 0.32));
    m_fretboard.setBounds (r.removeFromBottom (fretboardHeight));
    r.removeFromBottom (Theme::gap);
    m_song.setBounds (r);

    m_strum.setBounds (bottom.removeFromLeft (bottom.getWidth () * 48 / 100));
    bottom.removeFromLeft (Theme::gap);
    if (m_transport != nullptr)
    {
        m_transport->setBounds (bottom.removeFromTop (40));
        bottom.removeFromTop (Theme::gap / 2);
    }
    m_export.setBounds (bottom);
}

juce::StringArray MainComponent::getMenuBarNames ()
{
    return { "File", "Settings", "Help" };
}

juce::PopupMenu MainComponent::getMenuForIndex (int index, const juce::String&)
{
    juce::PopupMenu menu;
    const bool      any = m_state.starter () != nullptr;
    if (index == 0)
    {
        menu.addItem (kGenerate, "Generate", true, false);
        menu.addItem (kSave, "Save to Library", any);
        menu.addSeparator ();
        menu.addItem (kExportMidi, "Export MIDI...", any);
        menu.addItem (kExportTab, "Export Tab...", any);
        menu.addItem (kExportJson, "Export JSON...", any);
        menu.addItem (kExportXml, "Export MusicXML...", any);
        if (onQuit)
        {
            menu.addSeparator ();
            menu.addItem (kQuit, "Quit");
        }
    }
    else if (index == 1)
    {
        if (onAudioSettings)
            menu.addItem (kAudioSettings, "Audio Settings...");

        // In A/B test mode, the menu does not show the table type (REQ-15-5).
        juce::PopupMenu tables;
        const bool      learned = m_state.settings ().tables == "learned";
        tables.addItem (kTablesRules, "Rule tables", !m_state.abTestMode (), !m_state.abTestMode () && !learned);
        tables.addItem (kTablesLearned, "Learned tables", !m_state.abTestMode (), !m_state.abTestMode () && learned);
        menu.addSubMenu ("Tables", tables);
        menu.addItem (kAbTest, "A/B Test Mode", true, m_state.abTestMode ());
        menu.addSeparator ();
        menu.addItem (kLeftHanded, "Left-handed", true, m_state.leftHanded ());
    }
    else
    {
        menu.addItem (kShortcuts, "Keyboard Shortcuts");
    }
    return menu;
}

void MainComponent::menuItemSelected (int itemId, int)
{
    switch (itemId)
    {
        case kGenerate: m_state.requestGenerate (); break;
        case kSave: m_export.saveToLibrary (); break;
        case kExportMidi: m_export.exportAs ("mid"); break;
        case kExportTab: m_export.exportAs ("tab"); break;
        case kExportJson: m_export.exportAs ("json"); break;
        case kExportXml: m_export.exportAs ("musicxml"); break;
        case kQuit:
            if (onQuit)
                onQuit ();
            break;
        case kAudioSettings:
            if (onAudioSettings)
                onAudioSettings ();
            break;
        case kTablesRules: m_state.setTables ("rules"); break;
        case kTablesLearned: m_state.setTables ("learned"); break;
        case kAbTest: m_state.setAbTestMode (!m_state.abTestMode ()); break;
        case kLeftHanded: m_state.setLeftHanded (!m_state.leftHanded ()); break;
        case kShortcuts:
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Keyboard Shortcuts", kShortcutText);
            break;
        default: break;
    }
}

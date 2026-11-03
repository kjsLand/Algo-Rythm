#include "SongView.h"

#include "ChordDiagram.h"
#include "Theme.h"

#include "algo/Nashville.h"
#include "algo/Pitch.h"
#include "algo/Render.h"

// One section tab with a drag handle on its left side.
class SongView::SectionTab : public juce::Button
{
public:
    SectionTab (SongView& owner, int index, const juce::String& name)
        : juce::Button (name),
          m_owner (owner),
          m_index (index)
    {
        setWantsKeyboardFocus (true);
        setTooltip ("Select " + name + " (" + juce::String (index + 1) + "). Drag the handle into your DAW to make a MIDI clip.");
    }

    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        const bool selected = m_owner.m_state.selectedSection () == m_index;
        auto       r        = getLocalBounds ().toFloat ().reduced (1.0f);
        g.setColour (selected ? Theme::panelRaised : (over ? Theme::panel.brighter (0.05f) : Theme::panel));
        g.fillRoundedRectangle (r, Theme::radius);
        if (selected)
        {
            g.setColour (Theme::accent);
            g.fillRect (r.removeFromBottom (3.0f));
        }

        // The drag handle: 3 short lines
        auto handle = handleArea ().toFloat ();
        g.setColour (Theme::textDim);
        for (int i = -1; i <= 1; ++i)
            g.fillRect (juce::Rectangle<float> (handle.getWidth () * 0.5f, 2.0f).withCentre (handle.getCentre ().translated (0.0f, i * 5.0f)));

        g.setColour (selected ? Theme::text : Theme::textDim);
        g.setFont (juce::FontOptions (15.0f, selected ? juce::Font::bold : juce::Font::plain));
        g.drawText (getName (), getLocalBounds ().withTrimmedLeft (handleArea ().getRight ()), juce::Justification::centred, true);

        if (hasKeyboardFocus (false))
        {
            g.setColour (Theme::accent);
            g.drawRoundedRectangle (getLocalBounds ().toFloat ().reduced (1.0f), Theme::radius, 1.0f);
        }
    }

    void clicked () override
    {
        m_owner.m_state.select (m_index, 0);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (m_dragging || !handleArea ().contains (e.getMouseDownPosition ()) || e.getDistanceFromDragStart () < 6)
        {
            juce::Button::mouseDrag (e);
            return;
        }
        if (!m_owner.makeSectionMidi)
            return;

        const auto file = m_owner.makeSectionMidi (m_index);
        if (!file.existsAsFile ())
            return;
        m_dragging = true;
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName () }, false, this, [this]
        {
            m_dragging = false;
        });
    }

private:
    juce::Rectangle<int> handleArea () const
    {
        return getLocalBounds ().removeFromLeft (22);
    }

    SongView& m_owner;
    int       m_index;
    bool      m_dragging { false };
};

// The bar cells of the selected section.
class SongView::BarGrid : public juce::Component
{
public:
    explicit BarGrid (SongView& owner)
        : m_owner (owner)
    {
        setWantsKeyboardFocus (true);
        setTitle ("Bars");
        setDescription ("Use the left and right arrow keys to select a bar.");
    }

    int columns () const
    {
        const auto* section = currentSection ();
        const int   bars    = section == nullptr ? 1 : static_cast<int> (section->bars.size ());
        return std::clamp (bars, 1, 8);
    }

    juce::Rectangle<float> cellBounds (int bar) const
    {
        const int   cols = columns ();
        const auto* sec  = currentSection ();
        const int   rows = sec == nullptr ? 1 : (static_cast<int> (sec->bars.size ()) + cols - 1) / cols;
        const float w    = getWidth () / static_cast<float> (cols);
        const float h    = getHeight () / static_cast<float> (std::max (1, rows));
        return { (bar % cols) * w, (bar / cols) * h, w, h };
    }

    const algo::Section* currentSection () const
    {
        const auto& starter = m_owner.m_shown;
        const int   index   = m_owner.m_state.selectedSection ();
        if (starter == nullptr || index < 0 || index >= static_cast<int> (starter->sections.size ()))
            return nullptr;
        return &starter->sections[static_cast<std::size_t> (index)];
    }

    void paint (juce::Graphics& g) override
    {
        const auto* section = currentSection ();
        const auto& starter = m_owner.m_shown;
        if (section == nullptr)
        {
            g.setColour (Theme::textDim);
            g.setFont (juce::FontOptions (16.0f));
            g.drawText ("Press Generate (G) to make a song starter.", getLocalBounds (), juce::Justification::centred);
            return;
        }

        for (int b = 0; b < static_cast<int> (section->bars.size ()); ++b)
        {
            const auto& bar      = section->bars[static_cast<std::size_t> (b)];
            auto        cell     = cellBounds (b).reduced (3.0f);
            const bool  selected = b == m_owner.m_state.selectedBar ();

            g.setColour (selected ? Theme::panelRaised : Theme::panel);
            g.fillRoundedRectangle (cell, Theme::radius);
            g.setColour (selected ? Theme::accent : Theme::outline);
            g.drawRoundedRectangle (cell, Theme::radius, selected ? 2.0f : 1.0f);

            auto content = cell.reduced (4.0f);
            g.setColour (Theme::textDim);
            g.setFont (juce::FontOptions (11.0f));
            g.drawText (juce::String (b + 1), content.withHeight (12.0f), juce::Justification::topLeft);

            const int chords = static_cast<int> (bar.chords.size ());
            for (int c = 0; c < chords; ++c)
            {
                auto        part   = content.withWidth (content.getWidth () / chords).withX (content.getX () + c * content.getWidth () / chords);
                const auto& symbol = bar.chords[static_cast<std::size_t> (c)];

                juce::String name, number;
                try
                {
                    name   = algo::chordName (symbol, starter->key, m_owner.m_state.data ());
                    number = algo::numeralText (symbol);
                }
                catch (const std::exception&)
                {
                    name = "?";
                }

                // In a short cell, the chord name and the Nashville number share one line.
                const bool  compact = part.getHeight () < 110.0f;
                const float nameH   = compact ? 16.0f : 20.0f;
                auto        line    = part.removeFromTop (nameH);
                g.setColour (Theme::text);
                g.setFont (juce::FontOptions (std::min (compact ? 14.0f : 18.0f, part.getWidth () * 0.22f), juce::Font::bold));
                if (compact)
                {
                    g.drawText (name, line.removeFromLeft (line.getWidth () * 0.6f), juce::Justification::centredRight);
                    g.setColour (Theme::textDim);
                    g.setFont (juce::FontOptions (12.0f));
                    g.drawText ("  " + number, line, juce::Justification::centredLeft);
                }
                else
                {
                    g.drawText (name, line, juce::Justification::centred);
                    g.setColour (Theme::textDim);
                    g.setFont (juce::FontOptions (13.0f));
                    g.drawText (number, part.removeFromTop (16.0f), juce::Justification::centred);
                }

                if (c < static_cast<int> (bar.voicings.size ()) && part.getHeight () >= 30.0f)
                    ChordDiagram::draw (g, part.reduced (1.0f), bar.voicings[static_cast<std::size_t> (c)], name, m_owner.m_state.leftHanded (), false);
            }
        }

        // The playback position
        if (m_owner.m_position && starter != nullptr)
        {
            const double tick = m_owner.m_position ();
            if (tick >= 0.0)
            {
                const auto starts = algo::sectionStarts (*starter);
                const int  index  = m_owner.m_state.selectedSection ();
                const auto start  = static_cast<double> (starts[static_cast<std::size_t> (index)]);
                const auto end    = static_cast<double> (starts[static_cast<std::size_t> (index) + 1]);
                if (tick >= start && tick < end)
                {
                    const double barTicks = static_cast<double> (algo::barTicks (starter->timeSig));
                    const int    bar      = static_cast<int> ((tick - start) / barTicks);
                    const double frac     = (tick - start) / barTicks - bar;
                    const auto   cell     = cellBounds (bar);
                    const float  x        = cell.getX () + static_cast<float> (frac) * cell.getWidth ();
                    g.setColour (Theme::playhead);
                    g.fillRect (juce::Rectangle<float> (x - 1.0f, cell.getY (), 2.0f, cell.getHeight ()));
                }
            }
        }

        if (hasKeyboardFocus (false))
        {
            g.setColour (Theme::accent.withAlpha (0.6f));
            g.drawRect (getLocalBounds (), 1);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto* section = currentSection ();
        if (section == nullptr)
            return;
        for (int b = 0; b < static_cast<int> (section->bars.size ()); ++b)
            if (cellBounds (b).contains (e.position))
                m_owner.m_state.select (m_owner.m_state.selectedSection (), b);
        grabKeyboardFocus ();
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        const auto* section = currentSection ();
        if (section == nullptr)
            return false;
        const int bars = static_cast<int> (section->bars.size ());
        const int bar  = m_owner.m_state.selectedBar ();
        if (key == juce::KeyPress::leftKey)
        {
            m_owner.m_state.select (m_owner.m_state.selectedSection (), (bar + bars - 1) % bars);
            return true;
        }
        if (key == juce::KeyPress::rightKey)
        {
            m_owner.m_state.select (m_owner.m_state.selectedSection (), (bar + 1) % bars);
            return true;
        }
        return false;
    }

private:
    SongView& m_owner;
};

SongView::SongView (AppState& state)
    : m_state (state)
{
    m_grid = std::make_unique<BarGrid> (*this);
    addAndMakeVisible (*m_grid);

    m_lockLabel.setText ("Locks:", juce::dontSendNotification);
    m_lockLabel.setColour (juce::Label::textColourId, Theme::textDim);
    addAndMakeVisible (m_lockLabel);

    const std::array<LockButton*, 3> locks { &m_lockChords, &m_lockVoicings, &m_lockStrum };
    for (int item = 0; item < 3; ++item)
    {
        addAndMakeVisible (locks[static_cast<std::size_t> (item)]);
        locks[static_cast<std::size_t> (item)]->onClick = [this, item, locks]
        {
            m_state.setSectionLock (m_state.selectedSection (), item, locks[static_cast<std::size_t> (item)]->getToggleState ());
        };
    }

    m_newProgression.setTooltip ("Make a new progression for this section only");
    m_newVoicing.setTooltip ("Choose other voicings for this section only");
    m_newStrum.setTooltip ("Make a new strum pattern for this section only");
    m_newProgression.onClick = [this]
    {
        m_state.requestReroll (algo::Stage::Chords, m_state.selectedSection ());
    };
    m_newVoicing.onClick = [this]
    {
        m_state.requestReroll (algo::Stage::Voicing, m_state.selectedSection ());
    };
    m_newStrum.onClick = [this]
    {
        m_state.requestReroll (algo::Stage::Strum, m_state.selectedSection ());
    };
    for (auto* b : { &m_newProgression, &m_newVoicing, &m_newStrum })
        addAndMakeVisible (b);

    m_state.addChangeListener (this);
    rebuild ();
    startTimerHz (30);
}

SongView::~SongView ()
{
    m_state.removeChangeListener (this);
}

void SongView::setPositionProvider (std::function<double ()> provider)
{
    m_position = std::move (provider);
}

void SongView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (m_shown != m_state.starter ())
        rebuild ();

    const int section = m_state.selectedSection ();
    m_lockChords.setToggleState (m_state.sectionLock (section, 0), juce::dontSendNotification);
    m_lockVoicings.setToggleState (m_state.sectionLock (section, 1), juce::dontSendNotification);
    m_lockStrum.setToggleState (m_state.sectionLock (section, 2), juce::dontSendNotification);
    for (auto& tab : m_tabs)
        tab->repaint ();
    repaint ();
    m_grid->repaint ();
}

void SongView::timerCallback ()
{
    if (m_position && m_position () >= 0.0)
        m_grid->repaint ();
}

void SongView::rebuild ()
{
    m_shown = m_state.starter ();
    m_tabs.clear ();
    if (m_shown != nullptr)
    {
        for (std::size_t i = 0; i < m_shown->sections.size (); ++i)
        {
            m_tabs.push_back (std::make_unique<SectionTab> (*this, static_cast<int> (i), juce::String (m_shown->sections[i].name)));
            addAndMakeVisible (*m_tabs.back ());
        }
    }
    const bool any = m_shown != nullptr;
    for (auto* c : std::initializer_list<juce::Component*> { &m_lockChords, &m_lockVoicings, &m_lockStrum, &m_newProgression, &m_newVoicing, &m_newStrum, &m_lockLabel })
        c->setVisible (any);
    resized ();
}

void SongView::paint (juce::Graphics& g)
{
    g.setColour (Theme::panel.darker (0.15f));
    g.fillRoundedRectangle (getLocalBounds ().toFloat (), Theme::radius);

    if (m_shown == nullptr)
        return;

    const auto&  s    = *m_shown;
    juce::String text = "Key: " + juce::String (algo::pitchClassName (s.key.root, s.key) + " " + s.key.scale) + "    BPM: " + juce::String (s.bpm) + "    Time: " + juce::String (s.timeSig.numerator) + "/" + juce::String (s.timeSig.denominator) + "    Capo: " + (s.capo > 0 ? juce::String (s.capo) : juce::String ("none")) + "    Tuning: " + juce::String (s.tuning.name) + "    " + juce::String (algo::difficultyName (s.difficulty));
    if (s.rating)
        text << "    Rating: " << *s.rating << "/5";

    g.setColour (Theme::text);
    g.setFont (juce::FontOptions (15.0f));
    g.drawText (text, getLocalBounds ().reduced (Theme::gap).removeFromTop (22), juce::Justification::centredLeft, true);
}

void SongView::resized ()
{
    auto r = getLocalBounds ().reduced (Theme::gap);
    r.removeFromTop (24);

    // One row: the section tabs on the left, the locks and the reroll buttons on the right.
    auto      row     = r.removeFromTop (30);
    const int buttonW = juce::jlimit (84, 130, row.getWidth () / 7);
    for (auto* button : { &m_newStrum, &m_newVoicing, &m_newProgression })
    {
        button->setBounds (row.removeFromRight (buttonW));
        row.removeFromRight (4);
    }
    row.removeFromRight (Theme::gap);
    for (auto* lock : { &m_lockStrum, &m_lockVoicings, &m_lockChords })
    {
        lock->setBounds (row.removeFromRight (26));
        row.removeFromRight (2);
    }
    m_lockLabel.setBounds (row.removeFromRight (row.getWidth () > 420 ? 50 : 0));
    row.removeFromRight (Theme::gap);

    if (!m_tabs.empty ())
    {
        const int w = std::min (170, row.getWidth () / static_cast<int> (m_tabs.size ()));
        for (auto& tab : m_tabs)
            tab->setBounds (row.removeFromLeft (w).reduced (2, 0));
    }

    const bool narrow = buttonW < 110;
    m_newProgression.setButtonText (narrow ? "New chords" : "New progression");

    r.removeFromTop (6);
    m_grid->setBounds (r);
}

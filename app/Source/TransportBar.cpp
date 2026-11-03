#include "TransportBar.h"

#include "Theme.h"

TransportBar::TransportBar (AppState& state, Sequencer& sequencer)
    : m_state (state),
      m_sequencer (sequencer)
{
    for (auto* c : std::initializer_list<juce::Component*> { &m_play, &m_stop, &m_loop, &m_click })
    {
        c->setWantsKeyboardFocus (true);
        addAndMakeVisible (c);
    }

    m_play.setTooltip ("Play from the start (Space)");
    m_stop.setTooltip ("Stop (Space)");
    m_loop.setTooltip ("Play the selected section again and again");
    m_click.setTooltip ("Turn the click on or off");
    m_click.setToggleState (m_sequencer.clickOn (), juce::dontSendNotification);

    m_play.onClick = [this]
    {
        m_sequencer.play ();
        update ();
    };
    m_stop.onClick = [this]
    {
        m_sequencer.stop ();
        update ();
    };
    m_loop.onClick = [this]
    {
        m_sequencer.setLoopSection (m_loop.getToggleState () ? m_state.selectedSection () : -1);
    };
    m_click.onClick = [this]
    {
        setClick (m_click.getToggleState ());
    };

    m_state.addChangeListener (this);
    startTimerHz (4);
}

TransportBar::~TransportBar ()
{
    m_state.removeChangeListener (this);
}

void TransportBar::togglePlay ()
{
    if (m_sequencer.isPlaying ())
        m_sequencer.stop ();
    else
        m_sequencer.play ();
    update ();
}

void TransportBar::setClick (bool on)
{
    m_sequencer.setClick (on);
    m_click.setToggleState (on, juce::dontSendNotification);
    if (onClickChanged)
        onClickChanged (on);
}

void TransportBar::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (m_loop.getToggleState ())
        m_sequencer.setLoopSection (m_state.selectedSection ());
}

void TransportBar::timerCallback ()
{
    update ();
}

void TransportBar::update ()
{
    m_play.setEnabled (m_state.starter () != nullptr);
    m_play.setColour (juce::TextButton::buttonColourId, m_sequencer.isPlaying () ? Theme::accent.darker (0.5f) : Theme::panelRaised);
}

void TransportBar::paint (juce::Graphics& g)
{
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (getLocalBounds ().toFloat (), Theme::radius);
}

void TransportBar::resized ()
{
    auto      r = getLocalBounds ().reduced (Theme::gap, 6);
    const int w = std::min (90, r.getWidth () / 5);
    m_play.setBounds (r.removeFromLeft (w));
    r.removeFromLeft (Theme::gap);
    m_stop.setBounds (r.removeFromLeft (w));
    r.removeFromLeft (Theme::gap);
    m_click.setBounds (r.removeFromRight (std::min (70, r.getWidth () / 3)));
    m_loop.setBounds (r);
}

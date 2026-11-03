#include "StrumView.h"

#include "Theme.h"

#include "algo/Json.h"
#include "algo/Strum.h"

StrumView::StrumView (AppState& state)
    : m_state (state)
{
    setTitle ("Strum pattern");
    m_state.addChangeListener (this);
}

StrumView::~StrumView ()
{
    m_state.removeChangeListener (this);
}

void StrumView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint ();
}

void StrumView::paint (juce::Graphics& g)
{
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (getLocalBounds ().toFloat (), Theme::radius);

    auto       r       = getLocalBounds ().toFloat ().reduced (Theme::gap);
    const auto starter = m_state.starter ();
    const int  index   = m_state.selectedSection ();

    g.setColour (Theme::textDim);
    g.setFont (juce::FontOptions (13.0f));
    auto title = r.removeFromTop (18.0f);

    if (starter == nullptr || index < 0 || index >= static_cast<int> (starter->sections.size ()))
    {
        g.drawText ("Strum", title, juce::Justification::centredLeft);
        return;
    }

    const auto&  section = starter->sections[static_cast<std::size_t> (index)];
    const auto&  strum   = section.strum;
    juce::String heading = juce::String (section.name) + " strum: " + juce::String (algo::strumText (strum));
    if (!strum.name.empty ())
        heading << "  (" << strum.name << ")";
    heading << "  " << strum.strings << " strings";
    g.drawText (heading, title, juce::Justification::centredLeft, true);

    const int   steps  = std::max (1, strum.steps);
    const float slotW  = r.getWidth () / steps;
    const auto  ruler  = juce::String (algo::strumRuler (strum, starter->timeSig)).removeCharacters (" ");
    auto        rulerR = r.removeFromBottom (18.0f);

    for (int i = 0; i < steps && i < static_cast<int> (strum.slots.size ()); ++i)
    {
        auto        cell   = juce::Rectangle<float> (r.getX () + i * slotW, r.getY (), slotW, r.getHeight ());
        const auto  stroke = strum.slots[static_cast<std::size_t> (i)];
        const float accent = i < static_cast<int> (strum.accent.size ()) ? static_cast<float> (strum.accent[static_cast<std::size_t> (i)]) : 0.0f;
        const float size   = std::min (cell.getWidth (), cell.getHeight ()) * (0.45f + 0.4f * accent);
        const auto  centre = cell.getCentre ();

        g.setColour (Theme::panelRaised);
        g.fillRect (cell.reduced (1.0f));

        juce::Path path;
        switch (stroke)
        {
            case algo::Stroke::Down: // An arrow that points down
                path.addArrow ({ centre.x, centre.y - size / 2, centre.x, centre.y + size / 2 }, size * 0.16f, size * 0.55f, size * 0.4f);
                g.setColour (Theme::accent);
                g.fillPath (path);
                break;
            case algo::Stroke::Up: // An arrow that points up
                path.addArrow ({ centre.x, centre.y + size / 2, centre.x, centre.y - size / 2 }, size * 0.16f, size * 0.55f, size * 0.4f);
                g.setColour (Theme::root);
                g.fillPath (path);
                break;
            case algo::Stroke::Mute: // A cross
                g.setColour (Theme::text);
                g.drawLine (centre.x - size / 3, centre.y - size / 3, centre.x + size / 3, centre.y + size / 3, 3.0f);
                g.drawLine (centre.x - size / 3, centre.y + size / 3, centre.x + size / 3, centre.y - size / 3, 3.0f);
                break;
            case algo::Stroke::Chuck: // A filled square
                g.setColour (Theme::warning);
                g.fillRect (juce::Rectangle<float> (size * 0.6f, size * 0.6f).withCentre (centre));
                break;
            case algo::Stroke::Pluck: // A circle with the string: 1 to 6, b (bass) or a (alternate bass)
                g.setColour (Theme::accent);
                g.drawEllipse (juce::Rectangle<float> (size * 0.8f, size * 0.8f).withCentre (centre), 2.0f);
                g.setFont (juce::FontOptions (size * 0.45f, juce::Font::bold));
                g.drawText (juce::String::charToString (algo::slotChar (strum, static_cast<std::size_t> (i))), juce::Rectangle<float> (size, size).withCentre (centre), juce::Justification::centred);
                break;
            case algo::Stroke::Rest: // A small dash
                g.setColour (Theme::textDim);
                g.fillRect (juce::Rectangle<float> (size * 0.4f, 2.0f).withCentre (centre));
                break;
        }

        g.setColour (i % (steps / std::max (1, starter->timeSig.numerator == 6 ? 2 : starter->timeSig.numerator)) == 0 ? Theme::text : Theme::textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (juce::String::charToString (ruler[i]), juce::Rectangle<float> (rulerR.getX () + i * slotW, rulerR.getY (), slotW, rulerR.getHeight ()), juce::Justification::centred);
    }
}

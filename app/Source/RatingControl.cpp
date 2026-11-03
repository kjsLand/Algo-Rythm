#include "RatingControl.h"

#include "Theme.h"

RatingControl::RatingControl ()
{
    setWantsKeyboardFocus (true);
    setTitle ("Rating");
    setTooltip ("Rate this song starter from 1 to 5. The rating saves it to the library.");
}

void RatingControl::setRating (int rating)
{
    m_rating = juce::jlimit (0, 5, rating);
    repaint ();
}

int RatingControl::starAt (juce::Point<float> p) const
{
    const float w = getWidth () / 5.0f;
    return juce::jlimit (1, 5, static_cast<int> (p.x / w) + 1);
}

void RatingControl::paint (juce::Graphics& g)
{
    const float w     = getWidth () / 5.0f;
    const int   shown = m_hover > 0 ? m_hover : m_rating;
    for (int i = 0; i < 5; ++i)
    {
        const auto  area = juce::Rectangle<float> (i * w, 0.0f, w, static_cast<float> (getHeight ())).reduced (3.0f);
        const float size = std::min (area.getWidth (), area.getHeight ());
        juce::Path  star;
        star.addStar (area.getCentre (), 5, size * 0.22f, size * 0.5f);
        g.setColour (i < shown ? Theme::warning : Theme::outline);
        g.fillPath (star);
    }
    if (hasKeyboardFocus (false))
    {
        g.setColour (Theme::accent);
        g.drawRect (getLocalBounds (), 1);
    }
}

void RatingControl::mouseMove (const juce::MouseEvent& e)
{
    m_hover = starAt (e.position);
    repaint ();
}

void RatingControl::mouseExit (const juce::MouseEvent&)
{
    m_hover = 0;
    repaint ();
}

void RatingControl::mouseUp (const juce::MouseEvent& e)
{
    setRating (starAt (e.position));
    if (onRate)
        onRate (m_rating);
}

bool RatingControl::keyPressed (const juce::KeyPress& key)
{
    const auto c = key.getTextCharacter ();
    if (c >= '1' && c <= '5')
    {
        setRating (c - '0');
        if (onRate)
            onRate (m_rating);
        return true;
    }
    return false;
}

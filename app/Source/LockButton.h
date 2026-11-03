#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

// A small toggle with an open lock icon and a closed lock icon (T14-02).
class LockButton : public juce::Button
{
public:
    explicit LockButton (const juce::String& what)
        : juce::Button ("Lock " + what)
    {
        setClickingTogglesState (true);
        setTooltip ("Lock the " + what + ". Generate does not change a locked item.");
        setWantsKeyboardFocus (true);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto       r      = getLocalBounds ().toFloat ().reduced (3.0f);
        const bool locked = getToggleState ();
        const auto colour = locked ? Theme::warning : (over || down ? Theme::text : Theme::textDim);

        const float size = std::min (r.getWidth (), r.getHeight ());
        auto        icon = juce::Rectangle<float> (size, size).withCentre (r.getCentre ());

        // The body of the lock
        auto body = icon.removeFromBottom (size * 0.55f);
        g.setColour (colour);
        g.fillRoundedRectangle (body, 2.0f);

        // The shackle: closed when locked, open (moved to the right) when not
        juce::Path  shackle;
        const float w = body.getWidth () * 0.62f;
        const float x = locked ? body.getCentreX () - w / 2 : body.getCentreX () - w / 2 + w * 0.45f;
        const float h = size * 0.42f;
        shackle.addArc (x, body.getY () - h, w, h * 2.0f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
        g.strokePath (shackle, juce::PathStrokeType (size * 0.12f));

        if (hasKeyboardFocus (false))
        {
            g.setColour (Theme::accent);
            g.drawRoundedRectangle (getLocalBounds ().toFloat ().reduced (0.5f), 3.0f, 1.0f);
        }
    }
};

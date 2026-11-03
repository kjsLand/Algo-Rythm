#pragma once

#include <juce_graphics/juce_graphics.h>

#include <cmath>

// The colours of the app. Each text colour has a contrast ratio of 4.5:1 or more on its background (REQ-11-6).
namespace Theme
{
    inline const juce::Colour background { 0xff1b1d23 };
    inline const juce::Colour panel { 0xff252832 };
    inline const juce::Colour panelRaised { 0xff2f3340 };
    inline const juce::Colour outline { 0xff4a5060 };
    inline const juce::Colour text { 0xffeeeef2 };
    inline const juce::Colour textDim { 0xffb4b8c6 };
    inline const juce::Colour accent { 0xff5cc8ff };    // Selection and the chord notes
    inline const juce::Colour root { 0xffffa45c };      // The root note
    inline const juce::Colour scale { 0xff3c4252 };     // The scale notes (fill)
    inline const juce::Colour scaleText { 0xffd4d8e4 }; // Text on a scale note
    inline const juce::Colour chordText { 0xff10131a }; // Text on a chord or root note
    inline const juce::Colour playhead { 0xffff5c7a };
    inline const juce::Colour warning { 0xffffd166 };
    inline const juce::Colour fretboard { 0xff3a2c22 };
    inline const juce::Colour fretWire { 0xffa8a8b0 };
    inline const juce::Colour stringColour { 0xffd8c8a8 };

    constexpr int gap    = 8;
    constexpr int radius = 6;

    // The WCAG relative luminance of a colour.
    inline double luminance (juce::Colour c)
    {
        const auto channel = [] (juce::uint8 v)
        {
            const double s = v / 255.0;
            return s <= 0.03928 ? s / 12.92 : std::pow ((s + 0.055) / 1.055, 2.4);
        };
        return 0.2126 * channel (c.getRed ()) + 0.7152 * channel (c.getGreen ()) + 0.0722 * channel (c.getBlue ());
    }

    // The WCAG contrast ratio of two colours, from 1 to 21.
    inline double contrast (juce::Colour a, juce::Colour b)
    {
        const double la = luminance (a), lb = luminance (b);
        return (std::max (la, lb) + 0.05) / (std::min (la, lb) + 0.05);
    }
}

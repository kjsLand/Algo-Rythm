#include "ChordDiagram.h"

#include "Theme.h"

#include "algo/Voicing.h"

#include <algorithm>

void ChordDiagram::setChord (const algo::Voicing& voicing, const juce::String& name)
{
    m_voicing = voicing;
    m_name    = name;
    setTitle ("Chord diagram " + name);
    repaint ();
}

void ChordDiagram::setLeftHanded (bool on)
{
    m_leftHanded = on;
    repaint ();
}

void ChordDiagram::setShowName (bool on)
{
    m_showName = on;
    repaint ();
}

void ChordDiagram::paint (juce::Graphics& g)
{
    draw (g, getLocalBounds ().toFloat (), m_voicing, m_name, m_leftHanded, m_showName);
}

int ChordDiagram::baseFret (const algo::Voicing& voicing)
{
    int lowest = 99, highest = 0;
    for (int fret : voicing.frets)
        if (fret > 0)
        {
            lowest  = std::min (lowest, fret);
            highest = std::max (highest, fret);
        }
    return highest <= 5 ? 1 : lowest;
}

void ChordDiagram::draw (juce::Graphics& g, juce::Rectangle<float> area, const algo::Voicing& voicing, const juce::String& name, bool leftHanded, bool showName)
{
    constexpr int kFrets = 5;
    // The diagram keeps its shape. A short area makes a narrower diagram.
    const float w = std::clamp (std::min (area.getWidth (), area.getHeight () / (showName ? 1.6f : 1.25f)), 30.0f, 160.0f);
    auto        r = juce::Rectangle<float> (w, std::min (area.getHeight (), w * (showName ? 1.6f : 1.25f))).withCentre (area.getCentre ());

    const float fontSize = std::max (9.0f, w * 0.13f);
    if (showName)
    {
        g.setColour (Theme::text);
        g.setFont (juce::FontOptions (fontSize * 1.15f, juce::Font::bold));
        g.drawText (name, r.removeFromTop (fontSize * 1.4f), juce::Justification::centred, false);
    }

    const int base     = baseFret (voicing);
    auto      markers  = r.removeFromTop (fontSize * 1.1f);
    auto      labelCol = r.removeFromLeft (w * 0.18f);
    r.removeFromRight (w * 0.08f);
    markers.removeFromLeft (labelCol.getWidth ());
    markers.setWidth (r.getWidth ());

    const float stringGap = r.getWidth () / (algo::kNumStrings - 1);
    const float fretGap   = r.getHeight () / kFrets;
    const auto  xOf       = [&] (int s)
    {
        const int column = leftHanded ? algo::kNumStrings - 1 - s : s;
        return r.getX () + column * stringGap;
    };

    // The frets and the nut
    g.setColour (Theme::textDim);
    for (int f = 0; f <= kFrets; ++f)
        g.drawLine (r.getX (), r.getY () + f * fretGap, r.getRight (), r.getY () + f * fretGap, f == 0 && base == 1 ? std::max (3.0f, w * 0.04f) : 1.0f);
    for (int s = 0; s < algo::kNumStrings; ++s)
        g.drawLine (xOf (s), r.getY (), xOf (s), r.getBottom (), 1.0f);

    if (base > 1)
    {
        g.setColour (Theme::text);
        g.setFont (juce::FontOptions (fontSize));
        g.drawText (juce::String (base), labelCol.withHeight (fretGap).withY (r.getY ()), juce::Justification::centred, false);
    }

    const auto rowOf = [&] (int fret)
    {
        return fret - base + (base == 1 ? 0 : 1); // 1 = the first fret space
    };

    // The barre
    if (const auto barre = algo::findBarre (voicing))
    {
        const int row = rowOf (barre->fret);
        if (row >= 1 && row <= kFrets)
        {
            const float x1 = std::min (xOf (barre->firstString), xOf (barre->lastString));
            const float x2 = std::max (xOf (barre->firstString), xOf (barre->lastString));
            const float y  = r.getY () + (row - 0.5f) * fretGap;
            juce::Path  arc;
            arc.startNewSubPath (x1, y);
            arc.quadraticTo ((x1 + x2) / 2, y - fretGap * 0.55f, x2, y);
            g.setColour (Theme::accent);
            g.strokePath (arc, juce::PathStrokeType (std::max (2.0f, w * 0.035f)));
        }
    }

    // The dots, the finger numbers and the open and muted strings
    const float dot = std::min (stringGap, fretGap) * 0.78f;
    for (int s = 0; s < algo::kNumStrings; ++s)
    {
        const int  fret = voicing.frets[static_cast<std::size_t> (s)];
        const auto top  = juce::Rectangle<float> (dot, markers.getHeight ()).withCentre ({ xOf (s), markers.getCentreY () });
        g.setFont (juce::FontOptions (fontSize));
        if (fret == algo::kMutedString)
        {
            g.setColour (Theme::textDim);
            g.drawText ("X", top, juce::Justification::centred, false);
            continue;
        }
        if (fret == 0)
        {
            g.setColour (Theme::text);
            g.drawText ("O", top, juce::Justification::centred, false);
            continue;
        }

        const int row = rowOf (fret);
        if (row < 1 || row > kFrets)
            continue;
        const auto circle = juce::Rectangle<float> (dot, dot).withCentre ({ xOf (s), r.getY () + (row - 0.5f) * fretGap });
        g.setColour (Theme::accent);
        g.fillEllipse (circle);

        const int finger = voicing.fingers[static_cast<std::size_t> (s)];
        if (finger > 0)
        {
            g.setColour (Theme::chordText);
            g.setFont (juce::FontOptions (dot * 0.75f, juce::Font::bold));
            g.drawText (juce::String (finger), circle, juce::Justification::centred, false);
        }
    }
}

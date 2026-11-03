#include "FretboardView.h"

#include "Theme.h"

#include "algo/Fretboard.h"
#include "algo/Pitch.h"
#include "algo/Scale.h"
#include "algo/Voicing.h"

namespace
{
    constexpr int kShownFrets = 15;
}

FretboardView::FretboardView (AppState& state)
    : m_state (state)
{
    setTitle ("Fretboard");
    m_state.addChangeListener (this);
}

FretboardView::~FretboardView ()
{
    m_state.removeChangeListener (this);
}

void FretboardView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint ();
}

void FretboardView::paint (juce::Graphics& g)
{
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (getLocalBounds ().toFloat (), Theme::radius);

    const auto starter = m_state.starter ();
    auto       area    = getLocalBounds ().toFloat ().reduced (Theme::gap * 2.0f, Theme::gap);
    auto       legend  = area.removeFromBottom (16.0f);
    area.removeFromBottom (16.0f); // The fret numbers
    area.removeFromLeft (16.0f);
    area.removeFromTop (std::min (12.0f, area.getHeight () * 0.08f));

    const bool  left      = m_state.leftHanded ();
    const int   capo      = starter != nullptr ? starter->capo : 0;
    const float fretWidth = area.getWidth () / (kShownFrets + 1);
    const float stringGap = area.getHeight () / (algo::kNumStrings - 1);

    // Fret f (from the nut, with no capo) is at the column f. The left-handed view mirrors the columns.
    const auto xOfFret = [&] (float column)
    {
        const float x = area.getX () + column * fretWidth;
        return left ? area.getRight () - (x - area.getX ()) : x;
    };
    // The high E string is at the top.
    const auto yOfString = [&] (int s)
    {
        return area.getY () + (algo::kNumStrings - 1 - s) * stringGap;
    };

    g.setColour (Theme::fretboard);
    g.fillRect (area);

    g.setColour (Theme::fretWire);
    for (int f = 0; f <= kShownFrets; ++f)
    {
        const float x = xOfFret (f + 1.0f);
        g.drawLine (x, area.getY (), x, area.getBottom (), f == 0 ? 5.0f : 1.5f);
    }
    for (int dot : { 3, 5, 7, 9, 15 })
    {
        g.setColour (Theme::fretWire.withAlpha (0.35f));
        g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ xOfFret (dot + 0.5f), area.getCentreY () }));
    }
    g.setColour (Theme::stringColour);
    for (int s = 0; s < algo::kNumStrings; ++s)
        g.drawLine (area.getX (), yOfString (s), area.getRight (), yOfString (s), 1.0f + (algo::kNumStrings - 1 - s) * 0.25f);

    g.setColour (Theme::textDim);
    g.setFont (juce::FontOptions (11.0f));
    for (int f : { 3, 5, 7, 9, 12, 15 })
        g.drawText (juce::String (f), juce::Rectangle<float> (fretWidth, 14.0f).withCentre ({ xOfFret (f + 0.5f), area.getBottom () + 12.0f }), juce::Justification::centred);

    if (starter == nullptr)
        return;

    // The capo bar at its fret
    if (capo > 0)
    {
        g.setColour (Theme::warning.withAlpha (0.85f));
        g.fillRoundedRectangle (juce::Rectangle<float> (8.0f, area.getHeight () + 10.0f).withCentre ({ xOfFret (capo + 0.75f), area.getCentreY () }), 3.0f);
    }

    const algo::Fretboard board (starter->tuning, capo);

    const algo::Voicing* voicing = nullptr;
    algo::PitchClass     rootPc  = starter->key.root;
    const int            section = m_state.selectedSection ();
    const int            barIdx  = m_state.selectedBar ();
    if (section >= 0 && section < static_cast<int> (starter->sections.size ()))
    {
        const auto& bars = starter->sections[static_cast<std::size_t> (section)].bars;
        if (barIdx >= 0 && barIdx < static_cast<int> (bars.size ()) && !bars[static_cast<std::size_t> (barIdx)].voicings.empty ())
        {
            const auto& bar = bars[static_cast<std::size_t> (barIdx)];
            voicing         = &bar.voicings.front ();
            try
            {
                rootPc = algo::concreteChord (bar.chords.front (), starter->key, m_state.data ()).root;
            }
            catch (const std::exception&)
            {
            }
        }
    }

    const float dot      = std::min (fretWidth * 0.8f, stringGap * 0.9f);
    const auto  drawNote = [&] (int s, int fret, juce::Colour fill, juce::Colour textColour, bool outline)
    {
        const float column = fret == 0 ? capo + 0.45f : capo + fret + 0.5f;
        const auto  circle = juce::Rectangle<float> (dot, dot).withCentre ({ xOfFret (column), yOfString (s) });
        g.setColour (fill);
        g.fillEllipse (circle);
        if (outline)
        {
            g.setColour (Theme::outline);
            g.drawEllipse (circle, 1.0f);
        }
        g.setColour (textColour);
        g.setFont (juce::FontOptions (dot * 0.48f, juce::Font::bold));
        g.drawText (juce::String (algo::pitchClassName (algo::pitchClassOf (board.midiNoteAt (s, fret)), starter->key)), circle, juce::Justification::centred);
    };

    // The scale notes of the key
    for (int s = 0; s < algo::kNumStrings; ++s)
        for (int fret = 0; fret + capo <= kShownFrets; ++fret)
        {
            const bool inVoicing = voicing != nullptr && voicing->frets[static_cast<std::size_t> (s)] == fret;
            if (!inVoicing && algo::isInScale (algo::pitchClassOf (board.midiNoteAt (s, fret)), starter->key, m_state.data ()))
                drawNote (s, fret, Theme::scale, Theme::scaleText, true);
        }

    // The voicing notes, with the root in the root colour
    if (voicing != nullptr)
        for (int s = 0; s < algo::kNumStrings; ++s)
        {
            const int fret = voicing->frets[static_cast<std::size_t> (s)];
            if (fret == algo::kMutedString || fret + capo > kShownFrets)
                continue;
            const bool isRoot = algo::pitchClassOf (board.midiNoteAt (s, fret)) == rootPc;
            drawNote (s, fret, isRoot ? Theme::root : Theme::accent, Theme::chordText, false);
        }

    // The legend
    const auto item = [&] (juce::Colour colour, const juce::String& text)
    {
        g.setColour (colour);
        g.fillEllipse (legend.removeFromLeft (12.0f).withSizeKeepingCentre (10.0f, 10.0f));
        legend.removeFromLeft (4.0f);
        g.setColour (Theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (text, legend.removeFromLeft (90.0f), juce::Justification::centredLeft);
    };
    item (Theme::root, "Root");
    item (Theme::accent, "Chord note");
    item (Theme::scale.brighter (0.4f), "Scale note");
    if (left)
        item (Theme::textDim, "Left-handed");
}

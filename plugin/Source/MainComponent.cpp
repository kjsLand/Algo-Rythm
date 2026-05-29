#include "MainComponent.h"

// =============================================================================
// Construction
// =============================================================================

MainComponent::MainComponent()
{
    setSize (600, 400);
    setWantsKeyboardFocus (true);
    addKeyListener (this);

    pickNext(); // prime the first pick immediately
}

MainComponent::~MainComponent()
{
    removeKeyListener (this);
}

// =============================================================================
// State
// =============================================================================

void MainComponent::pickNext()
{
    currentPicks = fb.pickRandomScaleNotes (scaleKey, scaleName, chordTypes, /*count=*/1);
    repaint();
}

// =============================================================================
// Input
// =============================================================================

bool MainComponent::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        pickNext();
        return true;
    }
    return false;
}

// =============================================================================
// Paint
// =============================================================================

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    if (currentPicks.empty())
        return;

    drawPick (g, currentPicks[0]);

    // Footer hint
    g.setFont (juce::FontOptions (12.0f));
    g.setColour (juce::Colours::grey);
    g.drawText ("Press Space or Return for a new note",
                getLocalBounds().removeFromBottom (28),
                juce::Justification::centred, false);
}

void MainComponent::drawPick (juce::Graphics& g,
                              const GuitarFretboard::ScaleNotePick& pick) const
{
    static const std::array<std::string, GuitarFretboard::NUM_STRINGS> STRING_NAMES =
        { "E2", "A2", "D3", "G3", "B3", "E4" };

    const int lineH  = 22;
    const int colType   = 10;
    const int colSym    = 120;
    const int colNotes  = 210;
    int y = 16;

    // ---- Header: picked note + location ----
    g.setFont (juce::FontOptions (20.0f));
    g.setColour (juce::Colours::yellow);

    const std::string fretLabel = pick.position.fret == 0
                                  ? "open"
                                  : "fret " + std::to_string (pick.position.fret);

    g.drawText (pick.noteName
                + "   ·   " + STRING_NAMES[pick.position.string]
                + "  "      + fretLabel,
                colType, y, getWidth() - colType, lineH + 4,
                juce::Justification::centredLeft, false);
    y += lineH + 10;

    // ---- Sub-header: scale label ----
    g.setFont (juce::FontOptions (13.0f));
    g.setColour (juce::Colours::lightblue);
    g.drawText (scaleKey + " " + scaleName + " scale",
                colType, y, getWidth(), lineH,
                juce::Justification::centredLeft, false);
    y += lineH + 4;

    // ---- Column headers ----
    g.setFont (juce::FontOptions (13.0f));
    g.setColour (juce::Colours::lightgrey);
    g.drawText ("Type",   colType,  y, 100, lineH, juce::Justification::centredLeft, false);
    g.drawText ("Symbol", colSym,   y, 80,  lineH, juce::Justification::centredLeft, false);
    g.drawText ("Notes",  colNotes, y, getWidth() - colNotes, lineH,
                juce::Justification::centredLeft, false);
    y += lineH;

    // Divider
    g.setColour (juce::Colours::grey);
    g.drawHorizontalLine (y, static_cast<float> (colType), static_cast<float> (getWidth() - 10));
    y += 4;

    // ---- One row per chord type ----
    g.setFont (juce::FontOptions (14.0f));

    for (std::size_t i = 0; i < pick.chords.size(); ++i)
    {
        const auto& cn = pick.chords[i];

        // Alternating row tint
        if (i % 2 == 0)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.fillRect (0, y, getWidth(), lineH);
        }

        // Build notes string
        std::string notesStr;
        for (std::size_t j = 0; j < cn.noteNames.size(); ++j)
        {
            if (j) notesStr += "  -  ";
            notesStr += cn.noteNames[j];
        }

        g.setColour (juce::Colours::white);
        g.drawText (cn.chordName, colType,  y, 100, lineH,
                    juce::Justification::centredLeft, false);

        g.setColour (juce::Colours::cyan);
        g.drawText (cn.symbol,    colSym,   y, 80, lineH,
                    juce::Justification::centredLeft, false);

        g.setColour (juce::Colours::white);
        g.drawText (notesStr,     colNotes, y, getWidth() - colNotes - 10, lineH,
                    juce::Justification::centredLeft, false);

        y += lineH;
    }
}

void MainComponent::resized() {}
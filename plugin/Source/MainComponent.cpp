#include "MainComponent.h"

// =============================================================================
// Static data
// =============================================================================

const std::vector<std::string> MainComponent::ALL_KEYS = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

static const std::vector<std::string> CHORD_CYCLE = {
    "Major", "Minor", "Maj7", "M7", "Dom7",
    "Sus2",  "Sus4",  "Add9", "Dim", "Dim7", "M7b5"
};

// =============================================================================
// Construction
// =============================================================================

MainComponent::MainComponent()
{
    setSize (860, 480);

    // ---- Buttons ----
    addAndMakeVisible (btnNewNote);
    addAndMakeVisible (btnNewChord);
    addAndMakeVisible (btnNewKey);
    addAndMakeVisible (btnNewStrum);

    btnNewNote .onClick = [this] { pickNewNote();  };
    btnNewChord.onClick = [this] { pickNewChord(); };
    btnNewKey  .onClick = [this] { pickNewKey();   };
    btnNewStrum.onClick = [this] { pickNewStrum(); };

    // Prime initial state
    pickNewNote();
    pickNewStrum();
}

MainComponent::~MainComponent() {}

// =============================================================================
// Actions
// =============================================================================

void MainComponent::pickNewNote()
{
    // Refresh scale pitch-class set
    scalePCs = GuitarFretboard::getScalePitchClasses (currentKey, currentScale);

    // Gather all in-scale positions on frets 0-12
    std::vector<GuitarFretboard::Position> pool;
    for (int s = 0; s < GuitarFretboard::NUM_STRINGS; ++s)
        for (int f = 0; f <= 12; ++f)
            if (scalePCs.count (fb.getMidiNote (s, f) % 12))
                pool.push_back ({ s, f });

    if (pool.empty()) return;

    std::uniform_int_distribution<int> dist (0, (int)pool.size() - 1);
    rootPos = pool[dist (rng)];
    voicing = fb.buildTightVoicing (rootPos, currentChordName);
    repaint();
}

void MainComponent::pickNewChord()
{
    // Cycle to a different chord type at random
    std::string next = currentChordName;
    if (CHORD_CYCLE.size() > 1)
    {
        std::uniform_int_distribution<int> dist (0, (int)CHORD_CYCLE.size() - 2);
        int idx = dist (rng);
        // Skip current chord
        for (const auto& c : CHORD_CYCLE)
            if (c == currentChordName) { if (idx >= (int)(&c - &CHORD_CYCLE[0])) ++idx; break; }
        next = CHORD_CYCLE[std::min (idx, (int)CHORD_CYCLE.size() - 1)];
    }
    currentChordName = next;
    voicing = fb.buildTightVoicing (rootPos, currentChordName);
    repaint();
}

void MainComponent::pickNewKey()
{
    std::string next = currentKey;
    if (ALL_KEYS.size() > 1)
    {
        std::uniform_int_distribution<int> dist (0, (int)ALL_KEYS.size() - 2);
        int idx = dist (rng);
        for (std::size_t i = 0; i < ALL_KEYS.size(); ++i)
            if (ALL_KEYS[i] == currentKey) { if (idx >= (int)i) ++idx; break; }
        next = ALL_KEYS[std::min (idx, (int)ALL_KEYS.size() - 1)];
    }
    currentKey = next;
    scalePCs   = GuitarFretboard::getScalePitchClasses (currentKey, currentScale);
    pickNewNote(); // also picks a new root in the new key
}

void MainComponent::pickNewStrum()
{
    std::uniform_int_distribution<unsigned int> seedDist;
    currentStrum = StrumPattern::generateRandom (seedDist (rng));
    repaint();
}

// =============================================================================
// Layout
// =============================================================================

void MainComponent::resized()
{
    const int btnH = 36;
    const int btnW = 120;
    const int pad  = 10;
    int x = pad;
    int y = getHeight() - btnH - pad;

    btnNewNote .setBounds (x,                y, btnW, btnH);
    btnNewChord.setBounds (x +   (btnW+pad), y, btnW, btnH);
    btnNewKey  .setBounds (x + 2*(btnW+pad), y, btnW, btnH);
    btnNewStrum.setBounds (x + 3*(btnW+pad), y, btnW, btnH);
}

// =============================================================================
// Paint
// =============================================================================

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    const int headerH  = 48;
    const int btnArea  = 56;   // buttons + padding at bottom
    const int legendH  = 20;
    const int strumH   = 52;   // strum pattern display
    const int pad      = 10;

    auto area      = getLocalBounds().reduced (pad);
    auto headerR   = area.removeFromTop (headerH);
    auto legendR   = area.removeFromBottom (legendH);
    area.removeFromBottom (btnArea - pad); // leave room for buttons
    auto strumR    = area.removeFromBottom (strumH);
    // remaining area = fretboard

    drawHeader    (g, headerR);
    drawFretboard (g, area);
    drawLegend    (g, legendR);
    drawStrumPanel(g, strumR);
}

// =============================================================================
// Header
// =============================================================================

void MainComponent::drawHeader (juce::Graphics& g, juce::Rectangle<int> r) const
{
    static const std::array<std::string, GuitarFretboard::NUM_STRINGS> STRING_NAMES =
        { "E2", "A2", "D3", "G3", "B3", "E4" };

    const std::string rootFull = fb.getNoteName (rootPos.string, rootPos.fret);
    const std::string fretLbl  = rootPos.fret == 0
                                 ? "open"
                                 : "fret " + std::to_string (rootPos.fret);

    g.setFont (juce::Font (juce::FontOptions (20.0f)).boldened());
    g.setColour (juce::Colours::yellow);
    g.drawText (rootFull + "  —  " + voicing.symbol,
                r.getX(), r.getY(), r.getWidth(), 26,
                juce::Justification::centredLeft, false);

    g.setFont (juce::FontOptions (13.0f));
    g.setColour (juce::Colours::lightblue);
    g.drawText (STRING_NAMES[rootPos.string] + "  " + fretLbl
                + "   |   " + currentKey + " " + currentScale,
                r.getX(), r.getY() + 26, r.getWidth(), 18,
                juce::Justification::centredLeft, false);
}

// =============================================================================
// Fretboard
// =============================================================================

void MainComponent::drawFretboard (juce::Graphics& g, juce::Rectangle<int> bounds) const
{
    static const std::array<std::string, GuitarFretboard::NUM_STRINGS> STRING_NAMES =
        { "E2", "A2", "D3", "G3", "B3", "E4" };
    static const std::set<int> INLAY_FRETS = { 3, 5, 7, 9, 12 };

    const int numFrets  = 13;  // 0-12
    const int labelColW = 32;
    const int fretNumH  = 18;

    const int bx = bounds.getX();
    const int by = bounds.getY();
    const int bw = bounds.getWidth();
    const int bh = bounds.getHeight() - fretNumH;

    const float colW = (float)(bw - labelColW) / numFrets;
    const float rowH = (float) bh / GuitarFretboard::NUM_STRINGS;
    const float dotR = std::min (colW, rowH) * 0.30f;

    // Build a set of {string,fret} positions that are in the voiced chord
    // (excluding root, which is handled separately)
    std::set<std::pair<int,int>> chordPositions;
    std::pair<int,int> rootKey = { rootPos.string, rootPos.fret };
    for (std::size_t i = 1; i < voicing.positions.size(); ++i)
        chordPositions.insert ({ voicing.positions[i].string, voicing.positions[i].fret });

    // ---- Nut ----
    float nutX = (float)(bx + labelColW) + colW * 0.5f;
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.drawLine (nutX, (float)by, nutX, (float)(by + bh), 3.0f);

    // ---- Fret lines ----
    for (int f = 1; f < numFrets; ++f)
    {
        float fx = (float)(bx + labelColW) + colW * (f + 0.5f);
        g.setColour (juce::Colours::grey.withAlpha (0.45f));
        g.drawLine (fx, (float)by, fx, (float)(by + bh), 1.0f);
    }

    // ---- Strings + labels ----
    for (int s = 0; s < GuitarFretboard::NUM_STRINGS; ++s)
    {
        int   row = GuitarFretboard::NUM_STRINGS - 1 - s; // high E at top
        float sy  = (float)by + rowH * row + rowH * 0.5f;

        g.setColour (juce::Colours::grey.withAlpha (0.4f));
        g.drawLine ((float)(bx + labelColW), sy,
                    (float)(bx + labelColW + bw - labelColW), sy, 1.0f);

        g.setFont (juce::FontOptions (11.0f));
        g.setColour (juce::Colours::lightgrey);
        g.drawText (STRING_NAMES[s], bx, (int)(sy - rowH * 0.5f),
                    labelColW - 2, (int)rowH,
                    juce::Justification::centredRight, false);
    }

    // ---- Inlay dots + fret numbers ----
    float midY = (float)by + (float)bh * 0.5f;
    for (int f = 0; f < numFrets; ++f)
    {
        float fx = (float)(bx + labelColW) + colW * (f + 0.5f);

        if (INLAY_FRETS.count (f))
        {
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.fillEllipse (fx - 4.f, midY - 4.f, 8.f, 8.f);
        }

        if (f == 0 || INLAY_FRETS.count (f))
        {
            g.setFont (juce::FontOptions (10.0f));
            g.setColour (juce::Colours::grey);
            g.drawText (std::to_string (f),
                        (int)(fx - colW * 0.5f), by + bh,
                        (int)colW, fretNumH,
                        juce::Justification::centred, false);
        }
    }

    // ---- Note dots ----
    for (int s = 0; s < GuitarFretboard::NUM_STRINGS; ++s)
    {
        int   row = GuitarFretboard::NUM_STRINGS - 1 - s;
        float sy  = (float)by + rowH * row + rowH * 0.5f;

        for (int f = 0; f < numFrets; ++f)
        {
            int pc = fb.getMidiNote (s, f) % 12;
            if (scalePCs.count (pc) == 0) continue;

            float fx = (float)(bx + labelColW) + colW * (f + 0.5f);

            std::pair<int,int> key = { s, f };
            bool isRoot   = (key == rootKey);
            bool isChord  = chordPositions.count (key) > 0;

            juce::Colour fill;
            if      (isRoot)  fill = juce::Colours::yellow;
            else if (isChord) fill = juce::Colours::orange;
            else              fill = juce::Colours::steelblue.withAlpha (0.6f);

            g.setColour (fill);
            g.fillEllipse (fx - dotR, sy - dotR, dotR * 2.f, dotR * 2.f);

            // Note label — pitch name without octave
            std::string label;
            for (char c : fb.getNoteName (s, f))
                if (!std::isdigit ((unsigned char)c) && c != '-') label += c;

            g.setFont (juce::FontOptions (dotR > 9.f ? 10.f : 8.f));
            g.setColour ((isRoot || isChord) ? juce::Colours::black
                                             : juce::Colours::white.withAlpha (0.8f));
            g.drawText (label,
                        (int)(fx - dotR), (int)(sy - dotR),
                        (int)(dotR * 2.f), (int)(dotR * 2.f),
                        juce::Justification::centred, false);
        }
    }
}

// =============================================================================
// Strum Panel
// =============================================================================

void MainComponent::drawStrumPanel (juce::Graphics& g, juce::Rectangle<int> bounds) const
{
    const auto& s = currentStrum;
    if (s.pattern.empty()) return;

    const int pad = 4;
    auto r = bounds.reduced (pad, 0);

    // ── background strip ──
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (r.toFloat(), 4.f);

    // ── label ──
    g.setFont (juce::FontOptions (11.0f));
    g.setColour (juce::Colours::lightgrey);
    juce::String info = juce::String (s.bpm) + " BPM  |  "
                      + juce::String (s.steps) + " steps  |  "
                      + "E(" + juce::String (s.strumHits) + "," + juce::String (s.steps) + ")"
                      + "  mask E(" + juce::String (s.restHits) + "," + juce::String (s.steps) + ")";
    g.drawText (info, r.getX(), r.getY(), r.getWidth(), 18,
                juce::Justification::centredLeft, false);

    // ── ruler + pattern ──
    const int cellW = std::max (1, (r.getWidth() - 60) / s.steps);
    const int startX = r.getX() + 56;
    const int rulerY = r.getY() + 18;
    const int patY   = r.getY() + 30;

    g.setFont (juce::FontOptions (10.0f));
    for (int i = 0; i < s.steps; ++i)
    {
        int cx = startX + i * cellW;

        // ruler char
        g.setColour (juce::Colours::grey);
        g.drawText (juce::String::charToString ((juce::juce_wchar) s.ruler[(size_t)i]),
                    cx, rulerY, cellW, 12, juce::Justification::centred, false);

        // pattern char — colour by type
        char c = s.pattern[(size_t)i];
        juce::Colour col;
        if      (c == 'D') col = juce::Colours::orange;
        else if (c == 'U') col = juce::Colours::cyan;
        else               col = juce::Colours::darkgrey;

        g.setColour (col);
        g.setFont (juce::Font (juce::FontOptions (12.0f)).boldened());
        g.drawText (juce::String::charToString ((juce::juce_wchar) c),
                    cx, patY, cellW, 14, juce::Justification::centred, false);
    }

    // ── legend labels ──
    g.setFont (juce::FontOptions (10.0f));
    g.setColour (juce::Colours::grey);
    g.drawText ("beat",    r.getX(), rulerY, 54, 12, juce::Justification::centredRight, false);
    g.drawText ("strum",   r.getX(), patY,   54, 14, juce::Justification::centredRight, false);
}

void MainComponent::drawLegend (juce::Graphics& g, juce::Rectangle<int> r) const
{
    struct Entry { juce::Colour col; std::string label; };
    const std::vector<Entry> entries = {
        { juce::Colours::yellow,                    "Root"       },
        { juce::Colours::orange,                    "Chord tone" },
        { juce::Colours::steelblue.withAlpha(0.6f), "Scale note" },
    };

    int x = r.getX();
    const int dotD = 12;
    const int gap  = 8;

    for (const auto& e : entries)
    {
        g.setColour (e.col);
        g.fillEllipse ((float)x, (float)(r.getCentreY() - dotD/2), (float)dotD, (float)dotD);
        g.setFont (juce::FontOptions (11.0f));
        g.setColour (juce::Colours::lightgrey);
        g.drawText (e.label, x + dotD + 4, r.getY(), 80, r.getHeight(),
                    juce::Justification::centredLeft, false);
        x += dotD + 4 + 80 + gap;
    }
}
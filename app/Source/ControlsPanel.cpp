#include "ControlsPanel.h"

#include "Theme.h"

#include "algo/Pitch.h"

namespace
{
    const juce::StringArray kKeys { "Any", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    const juce::StringArray kMoods { "Bright", "Sad", "Hopeful", "Bluesy" };
    const juce::StringArray kForms { "Verse-Chorus", "Verse-Chorus-Bridge", "Loop" };
    const juce::StringArray kDifficulties { "Beginner", "Intermediate", "Advanced" };

    void fill (juce::ComboBox& box, const juce::StringArray& items)
    {
        box.clear (juce::dontSendNotification);
        for (int i = 0; i < items.size (); ++i)
            box.addItem (items[i], i + 1);
    }

    void setupSlider (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setRange (algo::kMinBpm, algo::kMaxBpm, 2.0);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour (juce::Slider::textBoxTextColourId, Theme::text);
        slider.setColour (juce::Slider::textBoxOutlineColourId, Theme::outline);
    }
}

ControlsPanel::ControlsPanel (AppState& state)
    : m_state (state)
{
    fill (m_key, kKeys);
    fill (m_mood, kMoods);
    fill (m_form, kForms);
    fill (m_difficulty, kDifficulties);
    for (int capo = 0; capo <= 7; ++capo)
        m_capo.addItem (juce::String (capo), capo + 1);
    for (std::size_t i = 0; i < m_state.data ().tunings.size (); ++i)
        m_tuning.addItem (m_state.data ().tunings[i].name, static_cast<int> (i) + 1);
    m_time.addItem ("4/4", 1);
    m_time.addItem ("6/8", 2);

    addLabelled (m_key, m_keyLabel, "Key");
    addLabelled (m_mood, m_moodLabel, "Mood");
    addLabelled (m_form, m_formLabel, "Form");
    addLabelled (m_difficulty, m_difficultyLabel, "Difficulty");
    addLabelled (m_capo, m_capoLabel, "Capo");
    addLabelled (m_tuning, m_tuningLabel, "Tuning");
    addLabelled (m_time, m_timeLabel, "Time");

    setupSlider (m_bpmMin);
    setupSlider (m_bpmMax);
    addLabelled (m_bpmMin, m_bpmMinLabel, "BPM min");
    addLabelled (m_bpmMax, m_bpmMaxLabel, "BPM max");

    addLabelled (m_seed, m_seedLabel, "Seed");
    m_seed.setInputRestrictions (19, "0123456789");
    m_seed.setTooltip ("The seed of the current song starter. Type a seed and press Enter to generate it again.");
    m_seed.onReturnKey = [this]
    {
        const auto text = m_seed.getText ();
        if (text.isNotEmpty ())
            m_state.requestGenerate (static_cast<std::uint64_t> (text.getLargeIntValue ()));
    };

    for (auto* c : std::initializer_list<juce::Component*> { &m_hook, &m_copy, &m_generate, &m_lockKey, &m_lockScale, &m_lockTempo, &m_lockForm })
        addAndMakeVisible (c);

    m_hook.setTooltip ("Add a melody hook of 2 to 4 bars to the chorus");
    m_copy.setTooltip ("Copy the seed to the clipboard");
    m_generate.setTooltip ("Generate a new song starter (G)");
    m_generate.setColour (juce::TextButton::buttonColourId, Theme::accent.darker (0.6f));

    m_copy.onClick = [this]
    {
        juce::SystemClipboard::copyTextToClipboard (m_seed.getText ());
    };
    m_generate.onClick = [this]
    {
        m_state.requestGenerate ();
    };

    for (auto* box : { &m_key, &m_mood, &m_form, &m_difficulty, &m_capo, &m_tuning, &m_time })
        box->onChange = [this]
        {
            push ();
        };
    m_bpmMin.onValueChange = [this]
    {
        if (m_bpmMin.getValue () > m_bpmMax.getValue ())
            m_bpmMax.setValue (m_bpmMin.getValue (), juce::dontSendNotification);
        push ();
    };
    m_bpmMax.onValueChange = [this]
    {
        if (m_bpmMax.getValue () < m_bpmMin.getValue ())
            m_bpmMin.setValue (m_bpmMax.getValue (), juce::dontSendNotification);
        push ();
    };
    m_hook.onClick = [this]
    {
        push ();
    };
    for (auto* lock : { &m_lockKey, &m_lockScale, &m_lockTempo, &m_lockForm })
        lock->onClick = [this]
        {
            push ();
        };

    refresh ();
    m_state.addChangeListener (this);
}

ControlsPanel::~ControlsPanel ()
{
    m_state.removeChangeListener (this);
}

void ControlsPanel::addLabelled (juce::Component& control, juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setColour (juce::Label::textColourId, Theme::textDim);
    label.setJustificationType (juce::Justification::bottomLeft);
    label.attachToComponent (&control, false);
    control.setTitle (text);
    control.setWantsKeyboardFocus (true);
    addAndMakeVisible (control);
}

void ControlsPanel::refresh ()
{
    m_updating    = true;
    const auto& s = m_state.settings ();
    m_key.setSelectedId (s.keyRoot ? *s.keyRoot + 2 : 1, juce::dontSendNotification);
    m_mood.setSelectedId (static_cast<int> (s.mood) + 1, juce::dontSendNotification);
    m_form.setSelectedId (kForms.indexOf (juce::String (s.form)) + 1, juce::dontSendNotification);
    m_difficulty.setSelectedId (static_cast<int> (s.difficulty) + 1, juce::dontSendNotification);
    m_capo.setSelectedId (s.capo + 1, juce::dontSendNotification);
    m_time.setSelectedId (s.timeSig.numerator == 6 ? 2 : 1, juce::dontSendNotification);
    m_bpmMin.setValue (s.bpmMin, juce::dontSendNotification);
    m_bpmMax.setValue (s.bpmMax, juce::dontSendNotification);
    m_bpmMinLabel.setText ("BPM min: " + juce::String (s.bpmMin), juce::dontSendNotification);
    m_bpmMaxLabel.setText ("BPM max: " + juce::String (s.bpmMax), juce::dontSendNotification);
    m_hook.setToggleState (s.hook, juce::dontSendNotification);

    for (int i = 0; i < m_tuning.getNumItems (); ++i)
        if (m_tuning.getItemText (i) == juce::String (s.tuning))
            m_tuning.setSelectedItemIndex (i, juce::dontSendNotification);

    m_lockKey.setToggleState (s.locks.key, juce::dontSendNotification);
    m_lockScale.setToggleState (s.locks.scale, juce::dontSendNotification);
    m_lockTempo.setToggleState (s.locks.tempo, juce::dontSendNotification);
    m_lockForm.setToggleState (s.locks.form, juce::dontSendNotification);

    if (const auto starter = m_state.starter (); starter != nullptr && !m_seed.hasKeyboardFocus (true))
        m_seed.setText (juce::String (static_cast<juce::int64> (starter->seed)), juce::dontSendNotification);
    m_updating = false;
}

void ControlsPanel::push ()
{
    if (m_updating)
        return;

    auto& s   = m_state.settings ();
    s.keyRoot = m_key.getSelectedId () <= 1 ? std::nullopt : std::optional<algo::PitchClass> (m_key.getSelectedId () - 2);
    s.mood    = static_cast<algo::Mood> (std::max (0, m_mood.getSelectedId () - 1));
    s.scale.reset (); // The mood chooses the scale.
    s.form       = kForms[std::max (0, m_form.getSelectedId () - 1)].toStdString ();
    s.difficulty = static_cast<algo::Difficulty> (std::max (0, m_difficulty.getSelectedId () - 1));
    s.capo       = std::max (0, m_capo.getSelectedId () - 1);
    s.tuning     = m_tuning.getText ().isEmpty () ? std::string ("Standard") : m_tuning.getText ().toStdString ();
    s.timeSig    = m_time.getSelectedId () == 2 ? algo::TimeSig { 6, 8 } : algo::TimeSig { 4, 4 };
    s.bpmMin     = static_cast<int> (m_bpmMin.getValue ());
    s.bpmMax     = static_cast<int> (m_bpmMax.getValue ());
    m_bpmMinLabel.setText ("BPM min: " + juce::String (s.bpmMin), juce::dontSendNotification);
    m_bpmMaxLabel.setText ("BPM max: " + juce::String (s.bpmMax), juce::dontSendNotification);
    s.hook        = m_hook.getToggleState ();
    s.locks.key   = m_lockKey.getToggleState ();
    s.locks.scale = m_lockScale.getToggleState ();
    s.locks.tempo = m_lockTempo.getToggleState ();
    s.locks.form  = m_lockForm.getToggleState ();
    m_state.settingsChanged ();
}

void ControlsPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh ();
}

void ControlsPanel::paint (juce::Graphics& g)
{
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (getLocalBounds ().toFloat (), Theme::radius);
}

void ControlsPanel::resized ()
{
    auto       r      = getLocalBounds ().reduced (Theme::gap);
    const int  rowH   = 26;
    const int  labelH = 18;
    const auto lockW  = 22;

    auto row1 = r.removeFromTop (labelH + rowH).withTrimmedTop (labelH);
    r.removeFromTop (Theme::gap);
    auto row2 = r.removeFromTop (labelH + rowH).withTrimmedTop (labelH);

    const auto place = [] (juce::Rectangle<int>& row, juce::Component& c, int width, juce::Component* lock = nullptr)
    {
        c.setBounds (row.removeFromLeft (width));
        if (lock != nullptr)
            lock->setBounds (row.removeFromLeft (22));
        row.removeFromLeft (Theme::gap);
    };

    // Row 1: key, mood (scale), form, difficulty and the hook
    m_hook.setBounds (row1.removeFromRight (70));
    row1.removeFromRight (Theme::gap);
    const int unit = std::max (40, (row1.getWidth () - 3 * lockW - 4 * Theme::gap) / 6);
    place (row1, m_key, unit, &m_lockKey);
    place (row1, m_mood, unit * 3 / 2, &m_lockScale);
    place (row1, m_form, unit * 2, &m_lockForm);
    m_difficulty.setBounds (row1.removeFromLeft (std::min (row1.getWidth (), unit * 3 / 2)));

    // Row 2: capo, tuning, time, BPM, seed and Generate
    m_generate.setBounds (row2.removeFromRight (std::min (120, row2.getWidth () / 7)));
    row2.removeFromRight (Theme::gap);
    m_copy.setBounds (row2.removeFromRight (52));
    row2.removeFromRight (4);
    m_seed.setBounds (row2.removeFromRight (std::min (120, row2.getWidth () / 6)));
    row2.removeFromRight (Theme::gap * 2);

    place (row2, m_capo, 54);
    place (row2, m_tuning, std::min (140, row2.getWidth () / 4));
    place (row2, m_time, 60);
    m_lockTempo.setBounds (row2.removeFromRight (lockW));
    row2.removeFromRight (4);
    const int sliderW = (row2.getWidth () - Theme::gap) / 2;
    m_bpmMin.setBounds (row2.removeFromLeft (sliderW));
    row2.removeFromLeft (Theme::gap);
    m_bpmMax.setBounds (row2);
}

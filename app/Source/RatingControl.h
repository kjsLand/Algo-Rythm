#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

// 5 stars (T14-05). A click or the keys 1 to 5 set an integer rating from 1 to 5 (REQ-14-4).
class RatingControl : public juce::Component,
                      public juce::SettableTooltipClient
{
public:
    RatingControl ();

    void setRating (int rating); // 0 = no rating
    int  rating () const
    {
        return m_rating;
    }

    std::function<void (int)> onRate;

    void paint (juce::Graphics& g) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    int starAt (juce::Point<float> p) const;

    int m_rating { 0 };
    int m_hover { 0 };
};

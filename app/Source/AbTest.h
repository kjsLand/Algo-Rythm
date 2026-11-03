#pragma once

#include <juce_core/juce_core.h>

#include <string>

// The A/B test mode (T15-05). Each Generate click uses the rule tables or the learned tables at random.
// The app does not show the table type until the user saves a rating (REQ-15-5).
namespace AbTest
{
    // "rules" or "learned", each with probability 0.5.
    std::string chooseTables (juce::Random& random);
}

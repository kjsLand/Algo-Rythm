#include "AbTest.h"

namespace AbTest
{
    std::string chooseTables (juce::Random& random)
    {
        return random.nextBool () ? "learned" : "rules";
    }
}

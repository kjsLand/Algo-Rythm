#pragma once

#include <stdexcept>
#include <string>

namespace algo
{
    // A generator stage cannot make a result from its inputs.
    class GenerationError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };
}

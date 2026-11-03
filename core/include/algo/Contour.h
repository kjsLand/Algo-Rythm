#pragma once

#include "Rng.h"

#include <vector>

namespace algo
{
    constexpr int kMinCoefficient  = -3;
    constexpr int kMaxCoefficient  = 3;
    constexpr int kMinContourTerms = 3;
    constexpr int kMaxContourTerms = 5;

    // A random Fourier series, as in sandbox/rand_period.py:
    // f(x) = a0 + sum of a[k] cos ((k + 1) x) + b[k] sin ((k + 1) x).
    struct Contour
    {
        int              a0 { 0 };
        std::vector<int> a;
        std::vector<int> b;
    };

    // Chooses each coefficient as an integer from -3 to 3. If all coefficients except a0 are 0,
    // chooses them again, so the contour is never a flat line. Throws std::invalid_argument if n < 1.
    Contour randomContour (int n, Rng& rng);

    // Chooses n from 3 to 5.
    Contour randomContour (Rng& rng);

    // t is in the range 0 to 1. The function maps it to 0 to 2π.
    double evaluate (const Contour& contour, double t);

    inline bool operator== (const Contour& x, const Contour& y)
    {
        return x.a0 == y.a0 && x.a == y.a && x.b == y.b;
    }
}

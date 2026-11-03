#include "algo/Contour.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace algo
{
    namespace
    {
        constexpr double kTwoPi = 6.283185307179586476925286766559;
    }

    Contour randomContour (int n, Rng& rng)
    {
        if (n < 1)
            throw std::invalid_argument ("A contour needs 1 or more terms: " + std::to_string (n));

        Contour contour;
        contour.a0 = rng.uniformInt (kMinCoefficient, kMaxCoefficient);

        const auto isZero = [] (int value)
        {
            return value == 0;
        };
        do
        {
            contour.a.clear ();
            contour.b.clear ();
            for (int k = 0; k < n; ++k)
                contour.a.push_back (rng.uniformInt (kMinCoefficient, kMaxCoefficient));
            for (int k = 0; k < n; ++k)
                contour.b.push_back (rng.uniformInt (kMinCoefficient, kMaxCoefficient));
        } while (std::all_of (contour.a.begin (), contour.a.end (), isZero) && std::all_of (contour.b.begin (), contour.b.end (), isZero));

        return contour;
    }

    Contour randomContour (Rng& rng)
    {
        return randomContour (rng.uniformInt (kMinContourTerms, kMaxContourTerms), rng);
    }

    double evaluate (const Contour& contour, double t)
    {
        const double x     = kTwoPi * t;
        double       value = contour.a0;
        for (std::size_t k = 0; k < contour.a.size (); ++k)
            value += contour.a[k] * std::cos (static_cast<double> (k + 1) * x);
        for (std::size_t k = 0; k < contour.b.size (); ++k)
            value += contour.b[k] * std::sin (static_cast<double> (k + 1) * x);
        return value;
    }
}

#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include <stdexcept>

// Bjorklund's algorithm distributes 'hits' as evenly as possible
// across 'steps' to produce a Euclidean rhythm bitstring.
// Returns a vector of 1s (hits) and 0s (rests).
std::vector<int> bjorklund(int hits, int steps) {
    if (hits < 0 || steps <= 0 || hits > steps)
        throw std::invalid_argument("Require 0 <= hits <= steps and steps > 0");

    if (hits == 0) return std::vector<int>(steps, 0);
    if (hits == steps) return std::vector<int>(steps, 1);

    // Each "group" is a sequence. We start with 'hits' groups of [1]
    // and (steps - hits) groups of [0].
    std::vector<std::vector<int>> pattern(hits, {1});
    std::vector<std::vector<int>> remainder(steps - hits, {0});

    // Repeatedly distribute the remainder into the pattern
    while (remainder.size() > 1) {
        int distribute = std::min(pattern.size(), remainder.size());

        std::vector<std::vector<int>> newPattern;
        newPattern.reserve(distribute);

        for (int i = 0; i < distribute; ++i) {
            std::vector<int> merged = pattern[i];
            merged.insert(merged.end(), remainder[i].begin(), remainder[i].end());
            newPattern.push_back(std::move(merged));
        }

        // Whatever wasn't paired becomes the new remainder
        std::vector<std::vector<int>> newRemainder;
        if (pattern.size() > remainder.size()) {
            newRemainder.assign(pattern.begin() + distribute, pattern.end());
        } else {
            newRemainder.assign(remainder.begin() + distribute, remainder.end());
        }

        pattern   = std::move(newPattern);
        remainder = std::move(newRemainder);
    }

    // Flatten
    std::vector<int> result;
    result.reserve(steps);
    for (auto& group : pattern)
        result.insert(result.end(), group.begin(), group.end());
    for (auto& group : remainder)
        result.insert(result.end(), group.begin(), group.end());

    return result;
}

// Pretty-print the bitstring using 'X' for hits and '.' for rests
std::string toPattern(const std::vector<int>& bits) {
    std::string s;
    s.reserve(bits.size());
    for (int b : bits)
        s += (b ? 'X' : '.');
    return s;
}

int main() {
    // ── Classic Euclidean rhythms ──────────────────────────────────────────
    struct Example { int hits, steps; const char* name; };
    Example examples[] = {
        {3, 8,  "Tresillo (Cuban)"},
        {5, 8,  "Cinquillo (Cuban)"},
        {4, 12, "West African bell"},
        {7, 12, "African 12/8 bell"},
        {5, 16, "Bossa Nova"},
        {3, 4,  "Simple 3-in-4"},
        {4, 9,  "Aksak (Turkish)"},
        {2, 5,  "Persian / Balkan"},
    };

    std::cout << "=== Euclidean Rhythms via Bjorklund's Algorithm ===\n\n";
    std::cout << "Format: E(hits, steps)  X=hit  .=rest\n\n";

    for (auto& ex : examples) {
        auto bits = bjorklund(ex.hits, ex.steps);
        std::cout << "E(" << ex.hits << "," << ex.steps << ")  "
                  << toPattern(bits)
                  << "  — " << ex.name << "\n";
    }

    // ── Interactive mode ───────────────────────────────────────────────────
    std::cout << "\n--- Enter your own (0 0 to quit) ---\n";
    int hits, steps;
    while (std::cout << "hits steps: " && std::cin >> hits >> steps) {
        if (hits == 0 && steps == 0) break;
        try {
            auto bits = bjorklund(hits, steps);
            std::cout << "E(" << hits << "," << steps << ")  "
                      << toPattern(bits) << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }

    return 0;
}
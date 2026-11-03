// Prints example Euclidean rhythms, then accepts your own from stdin.
// Build from the repository root: g++ -std=c++17 tools/bjorklund_demo.cpp -o bjorklund_demo
#include "../plugin/Source/Bjorklund.h"

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

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

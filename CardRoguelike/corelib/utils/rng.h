#pragma once

#include <random>

class RNG
{
public:
    static int nonDeterministicSeed();
    static void seed(int value);
    static int get(int min, int max);
    /// Uniform real in [min, max]. Seedable via seed().
    static double get(double min, double max);

    /// Snapshot / restore the global engine so callers can peek without
    /// permanently advancing mid-run RNG (e.g. run-boss assignment).
    static std::mt19937 snapshot();
    static void restore(const std::mt19937 &engine);

private:
    RNG() = default;
};

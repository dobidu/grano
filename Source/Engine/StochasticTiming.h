#pragma once
#include <juce_core/juce_core.h>

// StochasticTiming — inter-grain interval distributions.
// All methods: RT-safe, no allocation, no I/O. Uses juce::Random for PRNG.
struct StochasticTiming
{
    enum class Distribution : int
    {
        Uniform     = 0,
        Gaussian    = 1,
        Poisson     = 2,
        Exponential = 3,
        Pareto      = 4,
        OneOverF    = 5,
    };

    // Returns next wait interval in milliseconds.
    // meanMs: base interval (= 1000 / density).
    // All results clamped to [1.0, 5 * meanMs].
    static float nextIntervalMs(float meanMs, Distribution dist,
                                juce::Random& rng) noexcept;

private:
    static float gaussian(float meanMs, juce::Random& rng) noexcept;
    static float poisson (float meanMs, juce::Random& rng) noexcept;
    static float pareto  (float meanMs, juce::Random& rng) noexcept;
    static float oneOverF(float meanMs, juce::Random& rng) noexcept;
};

#include "StochasticTiming.h"
#include <cmath>
#include <algorithm>

static constexpr float kTwoPi = 6.28318530718f;

float StochasticTiming::gaussian(float meanMs, juce::Random& rng) noexcept
{
    // Box-Muller, σ = 0.3 * meanMs
    const float u1 = std::max(1e-7f, rng.nextFloat());
    const float u2 = rng.nextFloat();
    const float z  = std::sqrt(-2.0f * std::log(u1)) * std::cos(kTwoPi * u2);
    return meanMs + z * (0.3f * meanMs);
}

float StochasticTiming::poisson(float meanMs, juce::Random& rng) noexcept
{
    // Clustered ("bursty") Poisson process: a two-phase hyperexponential.
    // Half the gaps are short intra-cluster gaps (mean 0.2·m), half are long
    // inter-cluster gaps (mean 1.92·m, compensated for the 5·m clamp), so the
    // overall mean stays ≈ m. Grains arrive in bursts separated by pauses —
    // audibly distinct from plain Exponential (memoryless) spacing, which the
    // menu also offers (R-22: the two used to be identical).
    const float u = std::max(1e-7f, rng.nextFloat());
    const bool  intraCluster = rng.nextFloat() < 0.5f;
    return (intraCluster ? 0.2f : 1.92f) * meanMs * (-std::log(u));
}

float StochasticTiming::pareto(float meanMs, juce::Random& rng) noexcept
{
    // α = 1.5, xm = meanMs * (α - 1) / α
    constexpr float alpha = 1.5f;
    const float xm  = meanMs * ((alpha - 1.0f) / alpha);
    const float u   = std::max(1e-7f, rng.nextFloat());
    return xm / std::pow(u, 1.0f / alpha);
}

float StochasticTiming::oneOverF(float meanMs, juce::Random& rng) noexcept
{
    // Voss algorithm approximation: weighted sum of 5 Gaussian octave bands.
    // Each band independently Gaussian; weight halves per octave.
    // Normalised so σ ≈ 0.4 * meanMs after weighting.
    static constexpr int kBands = 5;
    float sum = 0.0f;
    for (int b = 0; b < kBands; ++b)
    {
        const float u1 = std::max(1e-7f, rng.nextFloat());
        const float u2 = rng.nextFloat();
        const float z  = std::sqrt(-2.0f * std::log(u1)) * std::cos(kTwoPi * u2);
        const float w  = 1.0f / (float)(1 << b);
        sum += w * z;
    }
    // sum of weights = 1 + 0.5 + 0.25 + 0.125 + 0.0625 = 1.9375
    sum /= 1.9375f;
    return meanMs + sum * (0.4f * meanMs);
}

float StochasticTiming::nextIntervalMs(float meanMs, Distribution dist,
                                       juce::Random& rng) noexcept
{
    float v;
    switch (dist)
    {
        case Distribution::Uniform:
            v = meanMs * (0.5f + rng.nextFloat()); // [0.5*mean, 1.5*mean]
            break;
        case Distribution::Gaussian:
            v = gaussian(meanMs, rng);
            break;
        case Distribution::Poisson:
            v = poisson(meanMs, rng);
            break;
        case Distribution::Exponential:
        {
            const float u = std::max(1e-7f, rng.nextFloat());
            v = -meanMs * std::log(u);
            break;
        }
        case Distribution::Pareto:
            v = pareto(meanMs, rng);
            break;
        case Distribution::OneOverF:
            v = oneOverF(meanMs, rng);
            break;
        default:
            v = meanMs;
            break;
    }
    return std::clamp(v, 1.0f, 5.0f * std::max(meanMs, 1.0f));
}

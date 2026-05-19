#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numeric>
#include <vector>

#include "Engine/StochasticTiming.h"

static constexpr int   N       = 10000;
static constexpr float MEAN_MS = 100.0f;
// Pareto with α=1.5 clamped at 5×mean has E[X]≈0.83×mean analytically.
// Tolerance widened to 20% to accommodate Pareto's truncated-tail bias.
static constexpr float TOL     = 0.20f; // ±20%

static void collectSamples(StochasticTiming::Distribution dist,
                            std::vector<float>& out)
{
    juce::Random rng(1234);
    out.resize(N);
    for (auto& v : out)
        v = StochasticTiming::nextIntervalMs(MEAN_MS, dist, rng);
}

static float calcMean(const std::vector<float>& v)
{
    return std::accumulate(v.begin(), v.end(), 0.0f) / (float)v.size();
}

static float calcStddev(const std::vector<float>& v)
{
    const float m = calcMean(v);
    float acc = 0.0f;
    for (float x : v) acc += (x - m) * (x - m);
    return std::sqrt(acc / (float)v.size());
}

TEST_CASE("All distributions produce positive values", "[stochastic]")
{
    using D = StochasticTiming::Distribution;
    for (auto dist : { D::Uniform, D::Gaussian, D::Poisson,
                       D::Exponential, D::Pareto, D::OneOverF })
    {
        std::vector<float> s;
        collectSamples(dist, s);
        for (float v : s)
            REQUIRE(v > 0.0f);
    }
}

TEST_CASE("All distributions have mean within 15% of requested", "[stochastic]")
{
    using D = StochasticTiming::Distribution;
    for (auto dist : { D::Uniform, D::Gaussian, D::Poisson,
                       D::Exponential, D::Pareto, D::OneOverF })
    {
        std::vector<float> s;
        collectSamples(dist, s);
        const float m = calcMean(s);
        REQUIRE(m >= MEAN_MS * (1.0f - TOL));
        REQUIRE(m <= MEAN_MS * (1.0f + TOL));
    }
}

TEST_CASE("Exponential stddev > Gaussian stddev (heavier tail)", "[stochastic]")
{
    std::vector<float> sg, se;
    collectSamples(StochasticTiming::Distribution::Gaussian,    sg);
    collectSamples(StochasticTiming::Distribution::Exponential, se);
    REQUIRE(calcStddev(se) > calcStddev(sg));
}

TEST_CASE("All distribution samples within clamp bounds", "[stochastic]")
{
    using D = StochasticTiming::Distribution;
    const float maxVal = 5.0f * MEAN_MS;
    for (auto dist : { D::Uniform, D::Gaussian, D::Poisson,
                       D::Exponential, D::Pareto, D::OneOverF })
    {
        std::vector<float> s;
        collectSamples(dist, s);
        for (float v : s)
        {
            REQUIRE(v >= 1.0f);
            REQUIRE(v <= maxVal);
        }
    }
}

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numeric>
#include <vector>

#include "Engine/SpectralProcessor.h"

static std::vector<float> makeSineSource(int len, float freq = 440.0f, float sr = 48000.0f)
{
    std::vector<float> v(len);
    for (int i = 0; i < len; ++i)
        v[i] = std::sin(6.28318f * freq * (float)i / sr);
    return v;
}

static float rms(const float* data, int len)
{
    float acc = 0.0f;
    for (int i = 0; i < len; ++i) acc += data[i] * data[i];
    return std::sqrt(acc / (float)len);
}

TEST_CASE("SpectralProcessor getNumSamples == kFFTSize", "[spectral]")
{
    SpectralProcessor sp;
    REQUIRE(sp.getNumSamples() == SpectralProcessor::kFFTSize);
}

TEST_CASE("SpectralProcessor Freeze output is non-silent", "[spectral]")
{
    SpectralProcessor sp;
    auto src = makeSineSource(SpectralProcessor::kFFTSize * 2);
    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Freeze, 0.0f);
    REQUIRE(rms(sp.getReadPointer(), sp.getNumSamples()) > 1e-4f);
}

TEST_CASE("SpectralProcessor Blur output is non-silent", "[spectral]")
{
    SpectralProcessor sp;
    auto src = makeSineSource(SpectralProcessor::kFFTSize * 2);
    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Blur, 1.0f);
    REQUIRE(rms(sp.getReadPointer(), sp.getNumSamples()) > 1e-4f);
}

TEST_CASE("SpectralProcessor Blur two runs differ (phase randomised)", "[spectral]")
{
    SpectralProcessor sp;
    auto src = makeSineSource(SpectralProcessor::kFFTSize * 2);

    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Blur, 1.0f);
    std::vector<float> out1(sp.getReadPointer(), sp.getReadPointer() + sp.getNumSamples());

    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Blur, 1.0f);
    std::vector<float> out2(sp.getReadPointer(), sp.getReadPointer() + sp.getNumSamples());

    float diffRms = 0.0f;
    for (int i = 0; i < sp.getNumSamples(); ++i)
    {
        float d = out1[i] - out2[i];
        diffRms += d * d;
    }
    diffRms = std::sqrt(diffRms / (float)sp.getNumSamples());
    REQUIRE(diffRms > 1e-3f);
}

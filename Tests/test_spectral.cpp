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

TEST_CASE("Spectral Freeze is a sustained texture, not a click", "[spectral]")
{
    // R-07: zero phase collapsed the frame into an impulse at n = 0, leaving
    // the middle of the buffer nearly silent.
    SpectralProcessor sp;
    auto src = makeSineSource(SpectralProcessor::kFFTSize * 2, 1000.0f);
    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Freeze, 0.0f);
    const float midRms = rms(sp.getReadPointer() + 512, 1024);
    REQUIRE(midRms >= 0.5f * rms(src.data(), SpectralProcessor::kFFTSize));

    // Deterministic: same source → same texture.
    std::vector<float> a(sp.getReadPointer(), sp.getReadPointer() + 2048);
    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Freeze, 0.0f);
    for (int i = 0; i < 2048; ++i)
        REQUIRE(sp.getReadPointer()[i] == a[(size_t)i]);
}

TEST_CASE("Spectral Blur 0 reproduces the source, Blur 1 smears it", "[spectral]")
{
    // R-08: blur replaced the phase, so blur = 0 was not the dry signal.
    SpectralProcessor sp;
    auto src = makeSineSource(SpectralProcessor::kFFTSize, 440.0f);
    auto corr = [&](const float* x)
    {
        double xy = 0, xx = 0, yy = 0;
        for (int i = 0; i < SpectralProcessor::kFFTSize; ++i)
        {
            xy += (double)x[i] * src[(size_t)i];
            xx += (double)x[i] * x[i];
            yy += (double)src[(size_t)i] * src[(size_t)i];
        }
        return xy / std::sqrt(xx * yy + 1e-20);
    };
    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Blur, 0.0f);
    REQUIRE(corr(sp.getReadPointer()) > 0.99);
    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Blur, 1.0f);
    REQUIRE(corr(sp.getReadPointer()) < 0.9);
}

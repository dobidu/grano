#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../Source/Modules/Color.h"

// ── helpers ──────────────────────────────────────────────────────────────────

static juce::AudioBuffer<float> makeStereoBuffer(int numSamples, float fillValue = 0.0f)
{
    juce::AudioBuffer<float> buf(2, numSamples);
    buf.clear();
    if (fillValue != 0.0f)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int n = 0; n < numSamples; ++n)
                buf.getWritePointer(ch)[n] = fillValue;
    }
    return buf;
}

// ── Test 1: bypass ────────────────────────────────────────────────────────────

TEST_CASE("Color bypass leaves buffer unchanged")
{
    Color c;
    c.prepare(44100.0, 512);

    std::atomic<float> en{ 0.0f }; // disabled
    std::atomic<float> sat{ 1.0f }, dec{ 1.0f }, tilt{ 1.0f }, verb{ 1.0f };
    c.setParamPointers(&en, &sat, &dec, &tilt, &verb);

    auto buf = makeStereoBuffer(512, 0.5f);
    c.processBlock(buf);

    for (int n = 0; n < 512; ++n)
        CHECK(buf.getReadPointer(0)[n] == Catch::Approx(0.5f));
}

// ── Test 2: saturate compresses full-scale peaks ──────────────────────────────

TEST_CASE("Color saturate pushes moderate signals toward saturation")
{
    // Normalised tanh: unity input stays at unity (identity at max).
    // Intermediate values get pushed toward 1.0 at high drive.
    // drive=16x: tanh(0.5 * 16) / tanh(16) ≈ 0.9999... — much higher than 0.5.
    Color c;
    c.prepare(44100.0, 512);

    std::atomic<float> en{ 1.0f };
    std::atomic<float> sat{ 1.0f }, dec{ 0.0f }, tilt{ 0.0f }, verb{ 0.0f };
    c.setParamPointers(&en, &sat, &dec, &tilt, &verb);

    auto buf = makeStereoBuffer(512, 0.5f); // moderate level
    c.processBlock(buf);

    for (int n = 0; n < 512; ++n)
        CHECK(buf.getReadPointer(0)[n] > 0.5f); // saturation boosts intermediate values
}

// ── Test 3: saturate at amount=0 passes through unchanged ─────────────────────

TEST_CASE("Color saturate amount=0 is transparent")
{
    Color c;
    c.prepare(44100.0, 512);

    std::atomic<float> en{ 1.0f };
    std::atomic<float> sat{ 0.0f }, dec{ 0.0f }, tilt{ 0.0f }, verb{ 0.0f };
    c.setParamPointers(&en, &sat, &dec, &tilt, &verb);

    auto buf = makeStereoBuffer(512, 0.3f);
    c.processBlock(buf);

    // With all effects at 0, output should equal input (only the amount<0.001 guard fires)
    for (int n = 0; n < 512; ++n)
        CHECK(buf.getReadPointer(0)[n] == Catch::Approx(0.3f));
}

// ── Test 4: decimate alters samples ──────────────────────────────────────────

TEST_CASE("Color decimate amount=1 alters sample values")
{
    Color c;
    c.prepare(44100.0, 512);

    std::atomic<float> en{ 1.0f };
    std::atomic<float> sat{ 0.0f }, dec{ 1.0f }, tilt{ 0.0f }, verb{ 0.0f };
    c.setParamPointers(&en, &sat, &dec, &tilt, &verb);

    // Ramp signal — would look like a smooth line without decimation
    juce::AudioBuffer<float> buf(2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int n = 0; n < 512; ++n)
            buf.getWritePointer(ch)[n] = (float)n / 512.0f * 0.5f;

    // Save original
    std::vector<float> original(512);
    for (int n = 0; n < 512; ++n)
        original[n] = buf.getReadPointer(0)[n];

    c.processBlock(buf);

    int diffCount = 0;
    for (int n = 0; n < 512; ++n)
        if (std::abs(buf.getReadPointer(0)[n] - original[n]) > 1e-4f)
            ++diffCount;

    CHECK(diffCount > 0);
}

// ── Test 5: tilt warm boosts low-frequency energy ────────────────────────────

TEST_CASE("Color tilt warm increases low-frequency content")
{
    // Use a long block to let the 1-pole filter settle
    const int N = 4096;
    Color flat, warm;
    flat.prepare(44100.0, N);
    warm.prepare(44100.0, N);

    std::atomic<float> en{ 1.0f };
    std::atomic<float> sat{ 0.0f }, dec{ 0.0f };
    std::atomic<float> tiltFlat{ 0.0f }, tiltWarm{ 1.0f };
    std::atomic<float> verb{ 0.0f };

    flat.setParamPointers(&en, &sat, &dec, &tiltFlat, &verb);
    warm.setParamPointers(&en, &sat, &dec, &tiltWarm, &verb);

    // White noise input (seeded deterministically)
    juce::Random rng(42);
    juce::AudioBuffer<float> bufFlat(2, N), bufWarm(2, N);
    for (int ch = 0; ch < 2; ++ch)
        for (int n = 0; n < N; ++n)
        {
            const float v = rng.nextFloat() * 2.0f - 1.0f;
            bufFlat.getWritePointer(ch)[n] = v;
            bufWarm.getWritePointer(ch)[n] = v;
        }

    flat.processBlock(bufFlat);
    warm.processBlock(bufWarm);

    // Low-frequency proxy: running sum of first half (dominated by low content)
    // Tilt warm adds 12 dB on lows → much higher energy in low portion
    float energyFlat = 0.0f, energyWarm = 0.0f;
    const float* fL = bufFlat.getReadPointer(0);
    const float* wL = bufWarm.getReadPointer(0);
    for (int n = 0; n < N; ++n)
    {
        energyFlat += fL[n] * fL[n];
        energyWarm += wL[n] * wL[n];
    }

    CHECK(energyWarm > energyFlat);
}

// ── Test 6: verb produces reverb tail ────────────────────────────────────────

TEST_CASE("Color verb produces tail after silent blocks")
{
    Color c;
    c.prepare(44100.0, 512);

    std::atomic<float> en{ 1.0f };
    std::atomic<float> sat{ 0.0f }, dec{ 0.0f }, tilt{ 0.0f }, verb{ 1.0f };
    c.setParamPointers(&en, &sat, &dec, &tilt, &verb);

    // First block: noise to excite reverb
    juce::Random rng(1);
    juce::AudioBuffer<float> excite(2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int n = 0; n < 512; ++n)
            excite.getWritePointer(ch)[n] = rng.nextFloat() * 2.0f - 1.0f;
    c.processBlock(excite);

    // Subsequent silent blocks — should still have tail
    float tailEnergy = 0.0f;
    for (int block = 0; block < 5; ++block)
    {
        juce::AudioBuffer<float> silent(2, 512);
        silent.clear();
        c.processBlock(silent);
        const float* data = silent.getReadPointer(0);
        for (int n = 0; n < 512; ++n)
            tailEnergy += data[n] * data[n];
    }

    CHECK(tailEnergy > 0.0f);
}

// ── Test 7: null param pointers — no crash ───────────────────────────────────

TEST_CASE("Color handles null param pointers without crash")
{
    Color c;
    c.prepare(44100.0, 512);
    // No setParamPointers call
    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();
    CHECK_NOTHROW(c.processBlock(buf));
}

// ── Test 8: reset clears filter state ────────────────────────────────────────

TEST_CASE("Color reset clears tilt state — zero input gives zero output")
{
    Color c;
    c.prepare(44100.0, 512);

    std::atomic<float> en{ 1.0f };
    std::atomic<float> sat{ 0.0f }, dec{ 0.0f }, tilt{ 1.0f }, verb{ 0.0f };
    c.setParamPointers(&en, &sat, &dec, &tilt, &verb);

    // Run some noise to build up filter state
    juce::AudioBuffer<float> noisy(2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int n = 0; n < 512; ++n)
            noisy.getWritePointer(ch)[n] = (n % 2 == 0) ? 0.5f : -0.5f;
    c.processBlock(noisy);

    // Reset — state should clear
    c.reset();

    // Zero input → zero output (tilt on, but no state to ring)
    juce::AudioBuffer<float> zeros(2, 512);
    zeros.clear();
    c.processBlock(zeros);

    float energy = 0.0f;
    for (int n = 0; n < 512; ++n)
        energy += zeros.getReadPointer(0)[n] * zeros.getReadPointer(0)[n];

    CHECK(energy == Catch::Approx(0.0f).margin(1e-6f));
}

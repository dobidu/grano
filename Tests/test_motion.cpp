#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../Source/Modules/Motion.h"

TEST_CASE("Motion default-constructs without error")
{
    Motion m;
    CHECK(m.getPitchModSemitones() == Catch::Approx(0.0f));
}

TEST_CASE("Motion reset zeroes pitch mod")
{
    Motion m;
    m.prepare(48000.0);
    m.reset();
    CHECK(m.getPitchModSemitones() == Catch::Approx(0.0f));
}

TEST_CASE("Motion disabled keeps pitch mod at zero")
{
    Motion m;
    m.prepare(48000.0);

    std::atomic<float> enabled{ 0.0f }; // off
    std::atomic<float> wowD{ 1.0f }, wowR{ 1.0f };
    std::atomic<float> fltD{ 1.0f }, fltR{ 8.0f };
    std::atomic<float> drift{ 1.0f };
    std::atomic<float> crackL{ -60.0f }, crackC{ 0.5f };

    m.setParamPointers(&enabled, &wowD, &wowR, &fltD, &fltR, &drift, &crackL, &crackC);

    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();
    for (int i = 0; i < 100; ++i)
        m.processBlock(buf);

    CHECK(m.getPitchModSemitones() == Catch::Approx(0.0f));
}

TEST_CASE("Motion pitch mod bounded when enabled with full wow")
{
    // Max wow = 50 cents = 0.5 semitones
    Motion m;
    m.prepare(48000.0);

    std::atomic<float> enabled{ 1.0f };
    std::atomic<float> wowD{ 1.0f }, wowR{ 1.0f };
    std::atomic<float> fltD{ 0.0f }, fltR{ 8.0f };
    std::atomic<float> drift{ 0.0f };
    std::atomic<float> crackL{ -60.0f }, crackC{ 0.5f };

    m.setParamPointers(&enabled, &wowD, &wowR, &fltD, &fltR, &drift, &crackL, &crackC);

    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();

    float maxSeen = 0.0f;
    for (int i = 0; i < 500; ++i)
    {
        m.processBlock(buf);
        maxSeen = std::max(maxSeen, std::abs(m.getPitchModSemitones()));
    }
    // wow-only: max 0.5 st + flutter/drift off
    CHECK(maxSeen <= 0.51f);
}

TEST_CASE("Motion crackle adds signal to silent buffer")
{
    Motion m;
    m.prepare(48000.0);

    std::atomic<float> enabled{ 1.0f };
    std::atomic<float> wowD{ 0.0f }, wowR{ 0.5f };
    std::atomic<float> fltD{ 0.0f }, fltR{ 8.0f };
    std::atomic<float> drift{ 0.0f };
    std::atomic<float> crackL{ -6.0f }, crackC{ 0.5f }; // loud crackle

    m.setParamPointers(&enabled, &wowD, &wowR, &fltD, &fltR, &drift, &crackL, &crackC);

    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();
    m.processBlock(buf);

    float energy = 0.0f;
    const float* data = buf.getReadPointer(0);
    for (int n = 0; n < 512; ++n)
        energy += data[n] * data[n];

    CHECK(energy > 0.0f);
}

TEST_CASE("Motion no null-pointer crash without param pointers")
{
    // Should not crash even with no setParamPointers call
    Motion m;
    m.prepare(44100.0);
    juce::AudioBuffer<float> buf(2, 256);
    buf.clear();
    CHECK_NOTHROW(m.processBlock(buf));
}

TEST_CASE("Motion prepare resets state each time")
{
    Motion m;
    m.prepare(44100.0);
    m.prepare(48000.0);
    CHECK(m.getPitchModSemitones() == Catch::Approx(0.0f));
}

// ── UAT-011 regression: crackle is audible at color = 0 ──────────────────────

TEST_CASE("Crackle produces output across the full color range", "[motion][crackle]")
{
    for (const float color : { 0.0f, 0.5f, 1.0f })
    {
        std::atomic<float> enabled{1.0f}, wowD{0.0f}, wowR{0.5f}, fltD{0.0f},
                           fltR{8.0f}, drift{0.0f}, crackL{0.0f} /*0 dB*/,
                           crackC{color};
        Motion m;
        m.setParamPointers(&enabled, &wowD, &wowR, &fltD, &fltR, &drift,
                           &crackL, &crackC);
        m.prepare(48000.0);

        juce::AudioBuffer<float> buf(2, 4096);
        buf.clear();
        m.processBlock(buf);

        float energy = 0.0f;
        for (int n = 0; n < 4096; ++n)
            energy += std::abs(buf.getSample(0, n));
        INFO("color=" << color);
        REQUIRE(energy > 0.0f);
    }
}

// ── 08-08 T6 ──────────────────────────────────────────────────────────────────

namespace
{
    struct MotionRig
    {
        std::atomic<float> en{1.0f}, wowD{0.0f}, wowR{0.5f}, flD{0.0f}, flR{8.0f},
                           drift{0.0f}, crk{-12.0f}, col{0.5f};
        Motion m;
        explicit MotionRig(double sr = 48000.0)
        {
            m.setParamPointers(&en, &wowD, &wowR, &flD, &flR, &drift, &crk, &col);
            m.prepare(sr);
        }
    };
}

TEST_CASE("Crackle level is roughly constant across the color knob", "[motion][crackle]")
{
    // R-17: color = 1 used to be ~34 dB quieter than color = 0.
    auto levelDb = [](float color)
    {
        MotionRig r;
        r.col.store(color);
        juce::AudioBuffer<float> buf(2, 512);
        double e = 0.0;
        for (int b = 0; b < 200; ++b)
        {
            buf.clear();
            r.m.processBlock(buf);
            for (int i = 0; i < 512; ++i) e += buf.getSample(0, i) * buf.getSample(0, i);
        }
        return 10.0 * std::log10(e / (200.0 * 512.0) + 1e-20);
    };
    const double ref = levelDb(0.0f);
    for (float c : { 0.25f, 0.5f, 0.75f, 1.0f })
    {
        INFO("color " << c);
        CHECK(std::abs(levelDb(c) - ref) < 6.0);
    }
}

TEST_CASE("Crackle is continuous across block boundaries on both channels",
          "[motion][crackle]")
{
    // R-16: one filter state shared by L and R stepped at every block edge.
    // Warm crackle (color 0) is strongly low-passed: sample-to-sample steps
    // are small, so a block-edge jump stands out.
    MotionRig r;
    r.col.store(0.0f);
    juce::AudioBuffer<float> buf(2, 64);
    float prev[2] = { 0.0f, 0.0f };
    double edgeJump = 0.0, innerJump = 0.0;
    int edges = 0, inner = 0;
    for (int b = 0; b < 400; ++b)
    {
        buf.clear();
        r.m.processBlock(buf);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 64; ++i)
            {
                const float v = buf.getSample(ch, i);
                const double d = std::abs(v - prev[ch]);
                if (i == 0 && b > 0) { edgeJump += d; ++edges; }
                else if (i > 0)      { innerJump += d; ++inner; }
                prev[ch] = v;
            }
    }
    CHECK(edgeJump / edges < 3.0 * innerJump / inner);
}

TEST_CASE("Drift timing does not depend on host block size", "[motion][drift]")
{
    // R-18: re-roll interval and smoothing were counted in blocks (32x
    // slower at 2048 than at 64). Measure mean |pitch change| per second.
    auto activity = [](int block)
    {
        MotionRig r;
        r.drift.store(1.0f);
        r.crk.store(-60.0f);
        juce::AudioBuffer<float> buf(2, block);
        const int blocksPerSec = 48000 / block;
        double total = 0.0;
        float last = 0.0f;
        for (int s = 0; s < 60; ++s)
        {
            for (int b = 0; b < blocksPerSec; ++b)
                r.m.processBlock(buf);
            const float now = r.m.getPitchModSemitones();
            total += std::abs(now - last);
            last = now;
        }
        return total / 60.0;
    };
    const double a64   = activity(64);
    const double a2048 = activity(2048);
    INFO("64: " << a64 << "  2048: " << a2048);
    CHECK(a64 > 0.01);   // drift actually moves (≥ 1 cent/s on average)
    CHECK(a2048 > 0.3 * a64);
    CHECK(a2048 < 3.0 * a64);
}

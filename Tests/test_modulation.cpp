#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Modulation/Lfo.h"
#include "PluginProcessor.h"

struct LfoFixture
{
    std::atomic<float> rate     { 1.0f };
    std::atomic<float> waveform { 0.0f }; // Sine
    std::atomic<float> sync     { 0.0f };
    std::atomic<float> phase    { 0.0f };
    std::atomic<float> depth    { 1.0f };
    Lfo lfo;

    LfoFixture()
    {
        lfo.setParamPointers(&rate, &waveform, &sync, &phase, &depth);
        lfo.prepare(44100.0);
    }
};

// ─── AC-1: Sine period ────────────────────────────────────────────────────────

TEST_CASE("Lfo Sine completes cycle: value crosses zero twice in one period")
{
    LfoFixture f;
    f.rate.store(1.0f);      // 1 Hz → 44100 samples per cycle
    f.waveform.store(0.0f);  // Sine

    // Run slightly past one full period to capture both crossings including the wrap
    int zeroCrossings = 0;
    float prev = f.lfo.advanceSample();
    for (int i = 1; i <= 44200; ++i)
    {
        const float cur = f.lfo.advanceSample();
        if ((prev >= 0.0f && cur < 0.0f) || (prev < 0.0f && cur >= 0.0f))
            ++zeroCrossings;
        prev = cur;
    }
    REQUIRE(zeroCrossings >= 2);
}

// ─── AC-2: Output ranges ──────────────────────────────────────────────────────

TEST_CASE("Lfo Sine output stays in [-1, +1]")
{
    LfoFixture f;
    f.waveform.store(0.0f);
    for (int i = 0; i < 1000; ++i)
    {
        const float v = f.lfo.advanceSample();
        REQUIRE(v >= -1.001f);
        REQUIRE(v <=  1.001f);
    }
}

TEST_CASE("Lfo Triangle output stays in [-1, +1]")
{
    LfoFixture f;
    f.waveform.store(1.0f);
    for (int i = 0; i < 1000; ++i)
    {
        const float v = f.lfo.advanceSample();
        REQUIRE(v >= -1.001f);
        REQUIRE(v <=  1.001f);
    }
}

TEST_CASE("Lfo Square outputs only +1 or -1")
{
    LfoFixture f;
    f.waveform.store(3.0f); // Square
    f.rate.store(10.0f);    // fast enough to see both states in 1000 samples

    for (int i = 0; i < 1000; ++i)
    {
        const float v = f.lfo.advanceSample();
        REQUIRE((v == Catch::Approx(1.0f) || v == Catch::Approx(-1.0f)));
    }
}

// ─── AC-3: S&H holds value within cycle ──────────────────────────────────────

TEST_CASE("Lfo SAndH holds value within one cycle")
{
    LfoFixture f;
    f.waveform.store(5.0f); // S&H
    f.rate.store(1.0f);     // 44100 samples per cycle; 100 samples is << half cycle

    const float first = f.lfo.advanceSample();
    bool allSame = true;
    for (int i = 1; i < 100; ++i)
    {
        if (f.lfo.advanceSample() != first)
        {
            allSame = false;
            break;
        }
    }
    REQUIRE(allSame);
}

// ─── AC-4: Drawable flat table ────────────────────────────────────────────────

TEST_CASE("Lfo Drawable flat 0.5 returns depth-scaled 0.5")
{
    LfoFixture f;
    f.waveform.store(6.0f); // Drawable
    f.depth.store(1.0f);

    for (int i = 0; i < Lfo::kDrawablePoints; ++i)
        f.lfo.setDrawablePoint(i, 0.5f);

    for (int i = 0; i < 200; ++i)
        REQUIRE(f.lfo.advanceSample() == Catch::Approx(0.5f).margin(0.001f));
}

TEST_CASE("Lfo Drawable setDrawablePoint / getDrawablePoint round-trip")
{
    Lfo lfo;
    lfo.prepare(44100.0);
    lfo.setDrawablePoint(0, 0.75f);
    lfo.setDrawablePoint(63, -0.3f);
    REQUIRE(lfo.getDrawablePoint(0)  == Catch::Approx(0.75f));
    REQUIRE(lfo.getDrawablePoint(63) == Catch::Approx(-0.3f));
}

// ─── AC-5: Depth scales output ────────────────────────────────────────────────

TEST_CASE("Lfo depth=0.5 halves Sine peak")
{
    LfoFixture f;
    f.waveform.store(0.0f); // Sine
    f.depth.store(0.5f);
    f.rate.store(10.0f);    // 10 Hz — see many cycles in 44100 samples

    float maxAbs = 0.0f;
    for (int i = 0; i < 44100; ++i)
        maxAbs = std::max(maxAbs, std::abs(f.lfo.advanceSample()));

    REQUIRE(maxAbs == Catch::Approx(0.5f).margin(0.01f));
}

// ─── AC-6: Phase offset ───────────────────────────────────────────────────────

TEST_CASE("Lfo phase offset 180 degrees shifts Sine: first sample near 0")
{
    LfoFixture f;
    f.waveform.store(0.0f); // Sine
    f.phase.store(180.0f);  // offset = π → sin(π) ≈ 0, going negative

    const float v = f.lfo.advanceSample();
    REQUIRE(std::abs(v) < 0.01f);
}

// ─── AC-7: Null param pointers ────────────────────────────────────────────────

TEST_CASE("Lfo null param pointers do not crash")
{
    Lfo lfo;
    lfo.prepare(44100.0);

    juce::AudioBuffer<float> buf(1, 512);
    buf.clear();

    lfo.processBlock(512, 120.0);
    for (int i = 0; i < 100; ++i)
        (void)lfo.advanceSample();

    REQUIRE(true);
}

// ─── AC-8: APVTS param count ──────────────────────────────────────────────────

TEST_CASE("APVTS contains 42 parameters after F5 LFO params added")
{
    GranoAudioProcessor proc;
    auto& apvts = proc.getAPVTS();

    // Spot-check new LFO params present
    REQUIRE(apvts.getParameter(ParamIDs::lfo1Rate)     != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::lfo1Waveform) != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::lfo1Sync)     != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::lfo1Phase)    != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::lfo1Depth)    != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::lfo2Rate)     != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::lfo2Waveform) != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::lfo2Sync)     != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::lfo2Phase)    != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::lfo2Depth)    != nullptr);

    // Total count = 42
    REQUIRE((int)proc.getParameters().size() == 42);
}

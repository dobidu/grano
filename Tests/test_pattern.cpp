#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Modules/Pattern.h"

// Helper: build a Pattern with all atomics set explicitly.
struct PatternFixture
{
    std::atomic<float> enabled         { 1.0f };
    std::atomic<float> triggerMode     { 0.0f }; // 0=Free
    std::atomic<float> syncDivision    { 1.0f }; // index 1 = 1/8
    std::atomic<float> euclidPulses    { 4.0f };
    std::atomic<float> euclidSteps     { 8.0f };
    std::atomic<float> euclidRotation  { 0.0f };
    std::atomic<float> transientSens   { 0.5f };
    std::atomic<float> probability     { 1.0f };
    std::atomic<float> reverseProb     { 0.0f };
    std::atomic<float> quantizeScale   { 0.0f }; // 0=Chromatic
    std::atomic<float> spray           { 0.0f };

    Pattern p;

    PatternFixture()
    {
        p.setParamPointers(&enabled, &triggerMode, &syncDivision,
                           &euclidPulses, &euclidSteps, &euclidRotation,
                           &transientSens, &probability, &reverseProb,
                           &quantizeScale, &spray);
        p.prepare(44100.0);
    }
};

// ─── AC-1: Bypass ─────────────────────────────────────────────────────────────

TEST_CASE("Pattern bypass - disabled always fires grains")
{
    PatternFixture f;
    f.enabled.store(0.0f);
    f.probability.store(0.0f); // would block everything if enabled

    int fires = 0;
    for (int i = 0; i < 100; ++i)
        if (f.p.shouldFireGrain()) ++fires;

    REQUIRE(fires == 100);
}

TEST_CASE("Pattern bypass - disabled never reverses grains")
{
    PatternFixture f;
    f.enabled.store(0.0f);
    f.reverseProb.store(1.0f); // would always reverse if enabled

    int reverses = 0;
    for (int i = 0; i < 100; ++i)
        if (f.p.shouldReverseGrain()) ++reverses;

    REQUIRE(reverses == 0);
}

TEST_CASE("Pattern bypass - disabled returns base pitch unchanged")
{
    PatternFixture f;
    f.enabled.store(0.0f);
    f.quantizeScale.store(1.0f); // Major - would snap if enabled

    const float base = 1.5f;
    for (int i = 0; i < 20; ++i)
        REQUIRE(f.p.getPitchAdjustment(base) == Catch::Approx(base));
}

// ─── AC-2: Probability ────────────────────────────────────────────────────────

TEST_CASE("Pattern probability=0 blocks all grains")
{
    PatternFixture f;
    f.probability.store(0.0f);

    int fires = 0;
    for (int i = 0; i < 100; ++i)
        if (f.p.shouldFireGrain()) ++fires;

    REQUIRE(fires == 0);
}

TEST_CASE("Pattern probability=1 passes all grains")
{
    PatternFixture f;
    f.probability.store(1.0f);

    int fires = 0;
    for (int i = 0; i < 100; ++i)
        if (f.p.shouldFireGrain()) ++fires;

    REQUIRE(fires == 100);
}

// ─── AC-3: Reverse Probability ───────────────────────────────────────────────

TEST_CASE("Pattern reverseProb=1 always reverses")
{
    PatternFixture f;
    f.reverseProb.store(1.0f);

    int reverses = 0;
    for (int i = 0; i < 100; ++i)
        if (f.p.shouldReverseGrain()) ++reverses;

    REQUIRE(reverses == 100);
}

TEST_CASE("Pattern reverseProb=0 never reverses")
{
    PatternFixture f;
    f.reverseProb.store(0.0f);

    int reverses = 0;
    for (int i = 0; i < 100; ++i)
        if (f.p.shouldReverseGrain()) ++reverses;

    REQUIRE(reverses == 0);
}

// ─── AC-4: Euclidean pattern ──────────────────────────────────────────────────

TEST_CASE("Pattern Euclidean 3-in-8 produces exactly 3 hits")
{
    PatternFixture f;
    f.triggerMode.store(2.0f);   // Euclidean
    f.euclidPulses.store(3.0f);
    f.euclidSteps.store(8.0f);
    f.euclidRotation.store(0.0f);
    f.probability.store(1.0f);

    int hits = 0;
    for (int i = 0; i < 8; ++i)
        if (f.p.shouldFireGrain()) ++hits;

    REQUIRE(hits == 3);
}

TEST_CASE("Pattern Euclidean 4-in-4 hits every step")
{
    PatternFixture f;
    f.triggerMode.store(2.0f);
    f.euclidPulses.store(4.0f);
    f.euclidSteps.store(4.0f);
    f.probability.store(1.0f);

    int hits = 0;
    for (int i = 0; i < 4; ++i)
        if (f.p.shouldFireGrain()) ++hits;

    REQUIRE(hits == 4);
}

TEST_CASE("Pattern Euclidean 1-in-8 produces exactly 1 hit per cycle")
{
    PatternFixture f;
    f.triggerMode.store(2.0f);
    f.euclidPulses.store(1.0f);
    f.euclidSteps.store(8.0f);
    f.probability.store(1.0f);

    int hits = 0;
    for (int i = 0; i < 8; ++i)
        if (f.p.shouldFireGrain()) ++hits;

    REQUIRE(hits == 1);
}

// ─── AC-5: Quantize ───────────────────────────────────────────────────────────

TEST_CASE("Pattern quantize Major snaps 1.0 st to 0 or 2")
{
    PatternFixture f;
    f.quantizeScale.store(1.0f); // Major: 0,2,4,5,7,9,11

    // 1.0 st is equidistant from 0 and 2; nearest is one of them.
    const float result = f.p.getPitchAdjustment(1.0f);
    REQUIRE((result == Catch::Approx(0.0f) || result == Catch::Approx(2.0f)));
}

TEST_CASE("Pattern quantize Chromatic leaves pitch unchanged")
{
    PatternFixture f;
    f.quantizeScale.store(0.0f); // Chromatic - no quantize applied (scaleIdx==0 skips)
    f.spray.store(0.0f);

    REQUIRE(f.p.getPitchAdjustment(1.5f) == Catch::Approx(1.5f));
}

TEST_CASE("Pattern quantize Major snaps 4.5 st to 4 or 5")
{
    PatternFixture f;
    f.quantizeScale.store(1.0f); // Major: ..., 4, 5, ...

    const float result = f.p.getPitchAdjustment(4.5f);
    REQUIRE((result == Catch::Approx(4.0f) || result == Catch::Approx(5.0f)));
}

// ─── AC-6: Spray ─────────────────────────────────────────────────────────────

TEST_CASE("Pattern spray=0 produces no pitch variation")
{
    PatternFixture f;
    f.spray.store(0.0f);
    f.quantizeScale.store(0.0f);

    const float base = 3.0f;
    for (int i = 0; i < 50; ++i)
        REQUIRE(f.p.getPitchAdjustment(base) == Catch::Approx(base));
}

TEST_CASE("Pattern spray=1 produces pitch variation > 1 semitone range")
{
    PatternFixture f;
    f.spray.store(1.0f);
    f.quantizeScale.store(0.0f);

    float minVal =  999.0f;
    float maxVal = -999.0f;
    for (int i = 0; i < 300; ++i)
    {
        const float v = f.p.getPitchAdjustment(0.0f);
        minVal = std::min(minVal, v);
        maxVal = std::max(maxVal, v);
    }
    REQUIRE((maxVal - minVal) > 1.0f);
}

// ─── AC-7: Sync interval ─────────────────────────────────────────────────────

TEST_CASE("Pattern Sync 120 BPM 1/8 note interval is 0.25s")
{
    PatternFixture f;
    f.triggerMode.store(1.0f);   // Sync
    f.syncDivision.store(1.0f);  // index 1 = 1/8

    juce::AudioBuffer<float> silent(1, 512);
    silent.clear();
    f.p.processBlock(silent, 120.0);

    REQUIRE(f.p.getNextIntervalSec() == Catch::Approx(0.25f).margin(0.001f));
}

TEST_CASE("Pattern Sync 120 BPM 1/4 note interval is 0.5s")
{
    PatternFixture f;
    f.triggerMode.store(1.0f);
    f.syncDivision.store(0.0f);  // index 0 = 1/4

    juce::AudioBuffer<float> silent(1, 512);
    silent.clear();
    f.p.processBlock(silent, 120.0);

    REQUIRE(f.p.getNextIntervalSec() == Catch::Approx(0.5f).margin(0.001f));
}

// ─── AC-8: Null param pointers safe ───────────────────────────────────────────

TEST_CASE("Pattern null param pointers do not crash")
{
    Pattern p;
    p.prepare(44100.0);

    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();

    p.processBlock(buf, 120.0);
    (void)p.shouldFireGrain();
    (void)p.shouldReverseGrain();
    (void)p.getPitchAdjustment(0.0f);
    (void)p.getDurMultiplier();
    (void)p.getNextIntervalSec();
    (void)p.isSchedulerOverrideActive();
    REQUIRE(true);
}

// ─── isSchedulerOverrideActive ───────────────────────────────────────────────

TEST_CASE("Pattern Free mode is not scheduler override")
{
    PatternFixture f;
    f.triggerMode.store(0.0f);
    REQUIRE(f.p.isSchedulerOverrideActive() == false);
}

TEST_CASE("Pattern Euclidean mode activates scheduler override")
{
    PatternFixture f;
    f.triggerMode.store(2.0f);
    REQUIRE(f.p.isSchedulerOverrideActive() == true);
}

TEST_CASE("Pattern disabled never overrides scheduler")
{
    PatternFixture f;
    f.enabled.store(0.0f);
    f.triggerMode.store(1.0f); // Sync but bypassed
    REQUIRE(f.p.isSchedulerOverrideActive() == false);
}

// ─── getDurMultiplier ─────────────────────────────────────────────────────────

TEST_CASE("Pattern spray=0 getDurMultiplier returns 1.0")
{
    PatternFixture f;
    f.spray.store(0.0f);
    for (int i = 0; i < 20; ++i)
        REQUIRE(f.p.getDurMultiplier() == Catch::Approx(1.0f));
}

TEST_CASE("Pattern spray=1 getDurMultiplier exceeds 1.0")
{
    PatternFixture f;
    f.spray.store(1.0f);

    float maxMult = 0.0f;
    for (int i = 0; i < 100; ++i)
        maxMult = std::max(maxMult, f.p.getDurMultiplier());

    REQUIRE(maxMult > 1.0f);
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Modulation/Lfo.h"
#include "Modulation/ModMatrix.h"
#include "Modulation/Snapshots.h"
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

// ─── AC-8: APVTS param count (updated for 05-02) ─────────────────────────────

TEST_CASE("APVTS contains 79 parameters after F6e envelopeShape param added")
{
    GranoAudioProcessor proc;
    auto& apvts = proc.getAPVTS();

    // LFO params present
    REQUIRE(apvts.getParameter(ParamIDs::lfo1Rate)     != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::lfo2Depth)    != nullptr);

    // Spot-check ModMatrix slot params
    REQUIRE(apvts.getParameter(ParamIDs::slot1Source)  != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::slot1Dest)    != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::slot1Amount)  != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::slot8Amount)  != nullptr);

    // F6b params present
    REQUIRE(apvts.getParameter(ParamIDs::subGrainDepth)  != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::stochasticDist) != nullptr);

    // F6c feedback params present
    REQUIRE(apvts.getParameter(ParamIDs::feedbackEnabled) != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::feedbackGain)    != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::feedbackDamp)    != nullptr);

    // F6c spectral params present
    REQUIRE(apvts.getParameter(ParamIDs::spectralEnabled)    != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::spectralMode)       != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::spectralBlurAmount) != nullptr);

    // F6d slot weight params present
    REQUIRE(apvts.getParameter(ParamIDs::slot0Weight) != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::slot1Weight) != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::slot2Weight) != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::slot3Weight) != nullptr);

    // F6e envelope shape param present; default = 0 (Hann)
    REQUIRE(apvts.getParameter(ParamIDs::envelopeShape) != nullptr);
    REQUIRE(apvts.getRawParameterValue(ParamIDs::envelopeShape)->load(std::memory_order_relaxed) == 0.0f);

    // Total count = 79 (78 prev + 1 F6e)
    REQUIRE((int)proc.getParameters().size() == 79);
}

// ─── ModMatrix ────────────────────────────────────────────────────────────────

struct ModFixture
{
    std::atomic<float> src    { 0.0f }; // None
    std::atomic<float> dst    { 0.0f }; // None
    std::atomic<float> amount { 0.0f };

    std::atomic<float> l1Rate { 1.0f };
    std::atomic<float> l1Wf   { 3.0f }; // Square — deterministic +1/-1 output
    std::atomic<float> l1Sync { 0.0f };
    std::atomic<float> l1Phase{ 0.0f };
    std::atomic<float> l1Depth{ 1.0f };

    Lfo lfo1;
    ModMatrix matrix;

    ModFixture()
    {
        lfo1.setParamPointers(&l1Rate, &l1Wf, &l1Sync, &l1Phase, &l1Depth);
        lfo1.prepare(44100.0);
        matrix.setLfos(&lfo1, nullptr);
        matrix.setSlotParams(0, &src, &dst, &amount);
        matrix.prepare(44100.0);
    }

    void oneBlock() { matrix.processBlock(512, 120.0); }
};

TEST_CASE("ModMatrix bypass: all slots None, offsets are zero")
{
    ModFixture f;
    f.oneBlock();
    REQUIRE(f.matrix.getModOffset(ModMatrix::kPitchShift)    == Catch::Approx(0.0f));
    REQUIRE(f.matrix.getModOffset(ModMatrix::kPositionJitter) == Catch::Approx(0.0f));
    REQUIRE(f.matrix.getModOffset(ModMatrix::kGrainSize)     == Catch::Approx(0.0f));
}

TEST_CASE("ModMatrix LFO1->kPitchShift: offset non-zero with Square LFO")
{
    ModFixture f;
    f.src.store(1.0f);                             // LFO1
    f.dst.store((float)ModMatrix::kPitchShift);
    f.amount.store(1.0f);

    float maxAbs = 0.0f;
    for (int i = 0; i < 20; ++i)
    {
        f.oneBlock();
        maxAbs = std::max(maxAbs, std::abs(f.matrix.getModOffset(ModMatrix::kPitchShift)));
    }
    REQUIRE(maxAbs > 0.5f);
}

TEST_CASE("ModMatrix amount scales output linearly")
{
    // Square LFO at t=0 is +1.0 (first half of cycle); use two separate fixtures
    ModFixture f05, f10;
    f05.src.store(1.0f); f05.dst.store((float)ModMatrix::kPitchShift); f05.amount.store(0.5f);
    f10.src.store(1.0f); f10.dst.store((float)ModMatrix::kPitchShift); f10.amount.store(1.0f);

    f05.oneBlock();
    f10.oneBlock();

    const float v05 = f05.matrix.getModOffset(ModMatrix::kPitchShift);
    const float v10 = f10.matrix.getModOffset(ModMatrix::kPitchShift);

    // Both started from same phase; ratio should be 0.5
    REQUIRE(std::abs(v10) > 0.01f);
    REQUIRE(v05 == Catch::Approx(v10 * 0.5f).margin(0.01f));
}

TEST_CASE("ModMatrix LFO2 source path produces non-zero offset")
{
    std::atomic<float> l2Rate{10.0f}, l2Wf{3.0f}, l2Sync{0.0f}, l2Phase{0.0f}, l2Depth{1.0f};
    Lfo lfo2;
    lfo2.setParamPointers(&l2Rate, &l2Wf, &l2Sync, &l2Phase, &l2Depth);
    lfo2.prepare(44100.0);

    ModFixture f;
    f.matrix.setLfos(&f.lfo1, &lfo2);
    f.src.store(2.0f); // LFO2
    f.dst.store((float)ModMatrix::kGrainSize);
    f.amount.store(1.0f);

    float maxAbs = 0.0f;
    for (int i = 0; i < 20; ++i)
    {
        f.oneBlock();
        maxAbs = std::max(maxAbs, std::abs(f.matrix.getModOffset(ModMatrix::kGrainSize)));
    }
    REQUIRE(maxAbs > 0.5f);
}

TEST_CASE("ModMatrix null LFOs do not crash and return zero")
{
    ModMatrix matrix;
    std::atomic<float> src{1.0f}, dst{(float)ModMatrix::kPitchShift}, amt{1.0f};
    matrix.setLfos(nullptr, nullptr);
    matrix.setSlotParams(0, &src, &dst, &amt);
    matrix.prepare(44100.0);

    for (int i = 0; i < 10; ++i)
        matrix.processBlock(512, 120.0);

    REQUIRE(matrix.getModOffset(ModMatrix::kPitchShift) == Catch::Approx(0.0f));
}

// ─── Snapshots ────────────────────────────────────────────────────────────────

TEST_CASE("Snapshots round-trip: save, modify, recall restores value")
{
    GranoAudioProcessor proc;
    auto& apvts = proc.getAPVTS();
    auto& snaps = proc.getSnapshots();

    if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(
            apvts.getParameter(ParamIDs::grainSize)))
        p->setValueNotifyingHost(p->convertTo0to1(50.0f));

    snaps.save(0, apvts.copyState());

    if (auto* p = dynamic_cast<juce::AudioParameterFloat*>(
            apvts.getParameter(ParamIDs::grainSize)))
        p->setValueNotifyingHost(p->convertTo0to1(200.0f));

    snaps.recall(0, apvts);

    const float restored = apvts.getRawParameterValue(ParamIDs::grainSize)->load();
    REQUIRE(restored == Catch::Approx(50.0f).margin(0.5f));
}

TEST_CASE("Snapshots isOccupied false before save, true after")
{
    GranoAudioProcessor proc;
    auto& snaps = proc.getSnapshots();
    REQUIRE(!snaps.isOccupied(0));
    REQUIRE(!snaps.isOccupied(3));
    snaps.save(0, proc.getAPVTS().copyState());
    REQUIRE(snaps.isOccupied(0));
    REQUIRE(!snaps.isOccupied(1));
}

TEST_CASE("ModMatrix cross-mod LFO1->kLfo2Rate accelerates LFO2")
{
    // LFO1: Square at +1, amount=1 → lfo2RateMod = +8000 Hz
    // LFO2: Sine at base 1 Hz; with +8000 Hz offset it completes many cycles quickly
    std::atomic<float> l2Rate{1.0f}, l2Wf{0.0f}, l2Sync{0.0f}, l2Phase{0.0f}, l2Depth{1.0f};
    Lfo lfo2;
    lfo2.setParamPointers(&l2Rate, &l2Wf, &l2Sync, &l2Phase, &l2Depth);
    lfo2.prepare(44100.0);

    ModFixture f;
    f.matrix.setLfos(&f.lfo1, &lfo2);
    f.src.store(1.0f);                              // LFO1
    f.dst.store((float)ModMatrix::kLfo2Rate);
    f.amount.store(1.0f);

    // Drive cross-mod for a few blocks
    for (int i = 0; i < 5; ++i)
        f.oneBlock();

    // LFO2 should have advanced many cycles; count zero-crossings in 1000 samples
    float prev = lfo2.advanceSample();
    int crossings = 0;
    for (int i = 1; i < 1000; ++i)
    {
        const float cur = lfo2.advanceSample();
        if ((prev >= 0.0f && cur < 0.0f) || (prev < 0.0f && cur >= 0.0f))
            ++crossings;
        prev = cur;
    }
    // At ~8001 Hz, ~181 crossings per 1000 samples; baseline 1 Hz would give ~0
    REQUIRE(crossings > 10);
}

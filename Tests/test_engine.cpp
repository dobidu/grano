#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <type_traits>
#include <unordered_set>

#include "Engine/EnvelopeShapes.h"
#include "Engine/Grain.h"
#include "Engine/GrainPool.h"
#include "Engine/GranularEngine.h"

// ── EnvelopeShapes ────────────────────────────────────────────────────────────

static constexpr EnvelopeShape kAllShapes[] = {
    EnvelopeShape::Hann,
    EnvelopeShape::Tukey,
    EnvelopeShape::Gaussian,
    EnvelopeShape::Triangle,
    EnvelopeShape::Rectangle,
};

TEST_CASE("EnvelopeShapes output in [0, 1] for all phases", "[dsp][envelope]")
{
    const float phases[] = { 0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 0.9f, 1.0f };
    for (auto shape : kAllShapes)
    {
        for (float p : phases)
        {
            const float v = applyEnvelope(shape, p);
            REQUIRE(v >= 0.0f);
            REQUIRE(v <= 1.0f);
        }
    }
}

TEST_CASE("EnvelopeShapes boundary values near zero for tapered shapes", "[dsp][envelope]")
{
    // Hann, Tukey, Triangle have hard zeros at boundaries (< 1%).
    const EnvelopeShape hardZero[] = {
        EnvelopeShape::Hann,
        EnvelopeShape::Tukey,
        EnvelopeShape::Triangle,
    };
    for (auto shape : hardZero)
    {
        REQUIRE(applyEnvelope(shape, 0.0f) < 0.01f);
        REQUIRE(applyEnvelope(shape, 1.0f) < 0.01f);
    }

    // Gaussian (σ=0.2 of full window) is a bell curve — boundary value is
    // exp(-0.5 * 2.5^2) ≈ 0.044. Not zero, but substantially below the peak.
    REQUIRE(applyEnvelope(EnvelopeShape::Gaussian, 0.0f) < 0.1f);
    REQUIRE(applyEnvelope(EnvelopeShape::Gaussian, 1.0f) < 0.1f);
}

TEST_CASE("EnvelopeShapes midpoint near peak for tapered shapes", "[dsp][envelope]")
{
    // Hann and Triangle peak exactly at 1.0
    REQUIRE(applyEnvelope(EnvelopeShape::Hann, 0.5f) == Catch::Approx(1.0f).margin(0.01f));
    REQUIRE(applyEnvelope(EnvelopeShape::Triangle, 0.5f) == Catch::Approx(1.0f).margin(0.01f));
    // Tukey is flat in the centre
    REQUIRE(applyEnvelope(EnvelopeShape::Tukey, 0.5f) == Catch::Approx(1.0f).margin(0.01f));
    // Gaussian peaks at 0.5 but value depends on σ
    REQUIRE(applyEnvelope(EnvelopeShape::Gaussian, 0.5f) >= 0.9f);
}

TEST_CASE("Rectangle returns 1.0 for all phases", "[dsp][envelope]")
{
    const float phases[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
    for (float p : phases)
        REQUIRE(applyEnvelope(EnvelopeShape::Rectangle, p) == Catch::Approx(1.0f));
}

TEST_CASE("EnvelopeShapes are symmetric around 0.5", "[dsp][envelope]")
{
    const EnvelopeShape symmetric[] = {
        EnvelopeShape::Hann,
        EnvelopeShape::Gaussian,
        EnvelopeShape::Triangle,
    };
    const float offsets[] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f };
    for (auto shape : symmetric)
    {
        for (float d : offsets)
        {
            const float left  = applyEnvelope(shape, 0.5f - d);
            const float right = applyEnvelope(shape, 0.5f + d);
            REQUIRE(left == Catch::Approx(right).margin(1e-5f));
        }
    }
}

// ── Grain ─────────────────────────────────────────────────────────────────────

TEST_CASE("Grain is trivially copyable", "[grain]")
{
    REQUIRE(std::is_trivially_copyable_v<Grain>);
}

TEST_CASE("Grain stores all fields correctly via aggregate init", "[grain]")
{
    float buf[64] = {};
    Grain g{ buf, 10, 64, 2048, 1.5f, -0.3f, EnvelopeShape::Triangle, 0.25f };

    REQUIRE(g.source        == buf);
    REQUIRE(g.startPos      == 10);
    REQUIRE(g.sourceLen     == 64);
    REQUIRE(g.lengthSamples == 2048);
    REQUIRE(g.pitchRatio    == Catch::Approx(1.5f));
    REQUIRE(g.pan           == Catch::Approx(-0.3f));
    REQUIRE(g.shape         == EnvelopeShape::Triangle);
    REQUIRE(g.currentPhase  == Catch::Approx(0.25f));
}

TEST_CASE("Grain default init is zero/null", "[grain]")
{
    Grain g{};
    REQUIRE(g.source        == nullptr);
    REQUIRE(g.startPos      == 0);
    REQUIRE(g.sourceLen     == 0);
    REQUIRE(g.lengthSamples == 0);
    REQUIRE(g.pitchRatio    == Catch::Approx(1.0f));
    REQUIRE(g.pan           == Catch::Approx(0.0f));
    REQUIRE(g.currentPhase  == Catch::Approx(0.0f));
}

// ── GrainPool ─────────────────────────────────────────────────────────────────

TEST_CASE("GrainPool acquires up to capacity", "[pool]")
{
    GrainPool pool;
    std::vector<Grain*> acquired;
    acquired.reserve(GrainPool::Capacity);

    for (int i = 0; i < GrainPool::Capacity; ++i)
    {
        Grain* g = pool.acquire();
        REQUIRE(g != nullptr);
        acquired.push_back(g);
    }

    REQUIRE(pool.activeCount() == GrainPool::Capacity);

    // Clean up
    for (auto* g : acquired)
        pool.release(g);
}

TEST_CASE("GrainPool returns nullptr when exhausted", "[pool]")
{
    GrainPool pool;
    std::vector<Grain*> acquired;
    acquired.reserve(GrainPool::Capacity);

    for (int i = 0; i < GrainPool::Capacity; ++i)
        acquired.push_back(pool.acquire());

    REQUIRE(pool.acquire() == nullptr);

    for (auto* g : acquired)
        pool.release(g);
}

TEST_CASE("GrainPool slot becomes reusable after release", "[pool]")
{
    GrainPool pool;

    // Fill pool
    std::vector<Grain*> acquired;
    for (int i = 0; i < GrainPool::Capacity; ++i)
        acquired.push_back(pool.acquire());

    REQUIRE(pool.acquire() == nullptr);

    // Release one slot
    pool.release(acquired[0]);
    acquired[0] = nullptr;

    // Should be acquirable again
    Grain* reused = pool.acquire();
    REQUIRE(reused != nullptr);

    // Clean up
    pool.release(reused);
    for (auto* g : acquired)
        if (g != nullptr) pool.release(g);
}

TEST_CASE("GrainPool all acquired pointers are distinct", "[pool]")
{
    GrainPool pool;
    std::unordered_set<Grain*> ptrs;

    for (int i = 0; i < GrainPool::Capacity; ++i)
    {
        Grain* g = pool.acquire();
        REQUIRE(g != nullptr);
        REQUIRE(ptrs.find(g) == ptrs.end()); // no duplicates
        ptrs.insert(g);
    }

    for (Grain* g : ptrs)
        pool.release(g);
}

TEST_CASE("GrainPool acquired grain is zero-initialised", "[pool]")
{
    GrainPool pool;
    Grain* g = pool.acquire();
    REQUIRE(g != nullptr);
    REQUIRE(g->source       == nullptr);
    REQUIRE(g->startPos     == 0);
    REQUIRE(g->currentPhase == Catch::Approx(0.0f));
    pool.release(g);
}

// ── Pitch ratio math ──────────────────────────────────────────────────────────
// Verifies the source-position formula used in GranularEngine::renderGrain.
// Tests pure math — no live audio required.

static float srcPosAtPhase(float phase, int grainLen, float pitchRatio) noexcept
{
    return phase * (float)grainLen * pitchRatio;
}

TEST_CASE("Pitch ratio 1.0 reads exactly lengthSamples source frames", "[grain][pitch]")
{
    const int N = 200;
    REQUIRE(srcPosAtPhase(0.0f, N, 1.0f) == Catch::Approx(0.0f));
    REQUIRE(srcPosAtPhase(1.0f, N, 1.0f) == Catch::Approx(200.0f));
    REQUIRE(srcPosAtPhase(0.5f, N, 1.0f) == Catch::Approx(100.0f));
}

TEST_CASE("Pitch ratio 2.0 reads twice as many source frames (pitch up)", "[grain][pitch]")
{
    const int N = 200;
    REQUIRE(srcPosAtPhase(1.0f, N, 2.0f) == Catch::Approx(400.0f));
    REQUIRE(srcPosAtPhase(0.5f, N, 2.0f) == Catch::Approx(200.0f));
}

TEST_CASE("Pitch ratio 0.5 reads half as many source frames (pitch down)", "[grain][pitch]")
{
    const int N = 200;
    REQUIRE(srcPosAtPhase(1.0f, N, 0.5f) == Catch::Approx(100.0f));
    REQUIRE(srcPosAtPhase(0.5f, N, 0.5f) == Catch::Approx(50.0f));
}

TEST_CASE("Pitch ratio 1.5 fractional source position is linear", "[grain][pitch]")
{
    const int N = 100;
    REQUIRE(srcPosAtPhase(0.0f,  N, 1.5f) == Catch::Approx(0.0f));
    REQUIRE(srcPosAtPhase(1.0f,  N, 1.5f) == Catch::Approx(150.0f));
    REQUIRE(srcPosAtPhase(0.25f, N, 1.5f) == Catch::Approx(37.5f));
}

// ── GranularEngine threading ──────────────────────────────────────────────────

TEST_CASE("GranularEngine runs without data races", "[engine][threading]")
{
    GranularEngine engine;
    engine.prepare(48000.0, 512);

    juce::AudioBuffer<float> buf(2, 512);

    // Give scheduler time to queue some grains (runs at ~50 Hz = 20 ms/grain)
    juce::Thread::sleep(100); // ~5 scheduler ticks

    for (int i = 0; i < 50; ++i)
    {
        buf.clear();
        engine.processBlock(buf);
    }

    engine.reset();
    // Reaching here under TSAN = no data races detected
    SUCCEED();
}

TEST_CASE("GranularEngine getGrainSnapshots returns plausible data", "[GranularEngine]")
{
    GranularEngine engine;
    engine.prepare(48000.0, 512);
    juce::Thread::sleep(100); // let scheduler fill grains from testSample_

    juce::AudioBuffer<float> out(2, 512);
    engine.processBlock(out); // writes snapshots

    std::array<GranularEngine::GrainSnapshot, GranularEngine::MaxActiveGrains> snaps;
    const int count = engine.getGrainSnapshots(snaps.data(), GranularEngine::MaxActiveGrains);

    REQUIRE(count >= 0);
    REQUIRE(count <= GranularEngine::MaxActiveGrains);
    for (int i = 0; i < count; ++i)
    {
        REQUIRE(snaps[i].srcFraction >= 0.0f);
        REQUIRE(snaps[i].srcFraction <= 1.0f);
        REQUIRE(snaps[i].envelopeAmp >= 0.0f);
        REQUIRE(snaps[i].envelopeAmp <= 1.0f);
    }
    engine.reset();
}

// ── UAT-003 / UAT-017 regressions ─────────────────────────────────────────────

#include "Engine/FeedbackPath.h"
#include "Engine/SpectralProcessor.h"
#include "Engine/MultiSampleBank.h"
#include "Modules/Pattern.h"

TEST_CASE("FeedbackPath and SpectralProcessor honor the +2 guard-sample contract",
          "[engine][guards]")
{
    // renderGrain interpolation may legally read source[len] and source[len+1].
    // Before the fix these two indices were past the end of both arrays (OOB).
    FeedbackPath fb;
    REQUIRE(fb.getReadPointer()[fb.getNumSamples()]     == 0.0f);
    REQUIRE(fb.getReadPointer()[fb.getNumSamples() + 1] == 0.0f);

    SpectralProcessor sp;
    std::array<float, SpectralProcessor::kFFTSize * 2> src{};
    src.fill(0.5f);
    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Freeze, 0.0f);
    REQUIRE(sp.getReadPointer()[sp.getNumSamples()]     == 0.0f);
    REQUIRE(sp.getReadPointer()[sp.getNumSamples() + 1] == 0.0f);

    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Blur, 1.0f);
    REQUIRE(sp.getReadPointer()[sp.getNumSamples()]     == 0.0f);
    REQUIRE(sp.getReadPointer()[sp.getNumSamples() + 1] == 0.0f);
}

TEST_CASE("Reversed grains with pitch-up near source end still render",
          "[engine][reverse]")
{
    // position 0.9 + 500 ms grains + pitch +12 st: reversed grains start with an
    // out-of-bounds read position. Before the fix they died at phase 0 (silence);
    // now they skip the OOB prefix and play the in-bounds remainder.
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 512;
    constexpr int    kFrames     = (int)kSampleRate;

    std::atomic<float> grainSize{500.0f}, density{50.0f}, position{0.9f},
                       posJitter{0.0f}, pitchShift{12.0f}, spread{0.5f},
                       masterVol{0.0f}, loop{0.0f};
    std::atomic<float> patEnabled{1.0f}, trigMode{0.0f}, syncDiv{0.0f},
                       euclidPulses{4.0f}, euclidSteps{8.0f}, euclidRot{0.0f},
                       transSens{0.5f}, probability{1.0f}, reverseProb{1.0f},
                       quantScale{0.0f}, spray{0.0f};

    Pattern pattern;
    pattern.setParamPointers(&patEnabled, &trigMode, &syncDiv, &euclidPulses,
                             &euclidSteps, &euclidRot, &transSens, &probability,
                             &reverseProb, &quantScale, &spray);
    pattern.prepare(kSampleRate);

    MultiSampleBank bank;
    auto buf = std::make_unique<juce::AudioBuffer<float>>(1, kFrames + 2);
    buf->clear();
    for (int i = 0; i < kFrames; ++i)
        buf->setSample(0, i, 0.5f);
    bank.loadSlot(0, std::move(buf), kFrames);

    GranularEngine engine;
    engine.setBank(&bank);
    engine.setParamPointers(&grainSize, &density, &position, &posJitter,
                            &pitchShift, &spread, &masterVol, &loop);
    engine.setPatternSource(&pattern);
    engine.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> out(2, kBlockSize);
    out.clear();
    engine.processBlock(out); // trySwapAll installs the slot

    juce::Thread::sleep(400); // let the scheduler queue grains

    float totalEnergy = 0.0f;
    for (int b = 0; b < 100; ++b)
    {
        out.clear();
        engine.processBlock(out);
        for (int s = 0; s < kBlockSize; ++s)
            totalEnergy += std::abs(out.getSample(0, s));
    }
    engine.reset();
    REQUIRE(totalEnergy > 0.0f);
}

// ── UAT-007 / UAT-008 regressions (loop sweep + core mod destinations) ────────

#include "Modulation/ModMatrix.h"

namespace
{
    struct EngineRig
    {
        std::atomic<float> grainSize{100.0f}, density{80.0f}, position{0.0f},
                           posJitter{0.0f}, pitchShift{0.0f}, spread{0.5f},
                           masterVol{0.0f}, loop{0.0f};
        MultiSampleBank bank;
        GranularEngine  engine;

        explicit EngineRig(double sampleRate = 48000.0, int frames = 48000)
        {
            auto buf = std::make_unique<juce::AudioBuffer<float>>(1, frames + 2);
            buf->clear();
            for (int i = 0; i < frames; ++i)
                buf->setSample(0, i, 0.5f);
            bank.loadSlot(0, std::move(buf), frames);

            engine.setBank(&bank);
            engine.setParamPointers(&grainSize, &density, &position, &posJitter,
                                    &pitchShift, &spread, &masterVol, &loop);
            engine.prepare(sampleRate, 512);
        }

        // Runs blocks over ~ms of wall time, returns max grain srcFraction seen.
        float maxSrcFractionOver(int ms, ModMatrix* matrix = nullptr)
        {
            juce::AudioBuffer<float> out(2, 512);
            std::array<GranularEngine::GrainSnapshot, 256> snaps{};
            float maxFrac = 0.0f;
            const auto tEnd = juce::Time::getMillisecondCounter() + (juce::uint32)ms;
            while (juce::Time::getMillisecondCounter() < tEnd)
            {
                if (matrix) matrix->processBlock(512, 120.0);
                out.clear();
                engine.processBlock(out);
                const int n = engine.getGrainSnapshots(snaps.data(), 256);
                for (int i = 0; i < n; ++i)
                    maxFrac = std::max(maxFrac, snaps[(size_t)i].srcFraction);
                juce::Thread::sleep(10);
            }
            return maxFrac;
        }
    };
}

TEST_CASE("Loop toggle sweeps grain position across the source", "[engine][loop]")
{
    // Loop OFF, position 0, jitter 0: grains stay at the start.
    {
        EngineRig rig;
        rig.loop.store(0.0f);
        const float maxFrac = rig.maxSrcFractionOver(700);
        rig.engine.reset();
        REQUIRE(maxFrac < 0.1f);
    }
    // Loop ON: after ~700 ms of a 1 s source, the sweep has moved well past 30%.
    {
        EngineRig rig;
        rig.loop.store(1.0f);
        const float maxFrac = rig.maxSrcFractionOver(700);
        rig.engine.reset();
        REQUIRE(maxFrac > 0.3f);
    }
}

TEST_CASE("ModMatrix kPosition destination moves grain positions", "[engine][moddest]")
{
    // amount 0.5: square pushes position to 0.5 (mid-sample) half the time —
    // grains there live a full 100 ms, long enough for the snapshot to see
    // them (amount 1.0 would clamp position to 1.0 → 1-sample grains that
    // finish before the snapshot pass).
    std::atomic<float> rate{20.0f}, wf{3.0f} /*Square*/, sync{0.0f}, phase{0.0f}, depth{1.0f};
    std::atomic<float> src{1.0f}, dst{(float)ModMatrix::kPosition}, amt{0.5f};
    Lfo lfo;
    lfo.setParamPointers(&rate, &wf, &sync, &phase, &depth);
    lfo.prepare(48000.0);
    ModMatrix matrix;
    matrix.setLfos(&lfo, nullptr);
    matrix.setSlotParams(0, &src, &dst, &amt);
    matrix.prepare(48000.0);

    EngineRig rig;
    rig.engine.reset();                      // stop scheduler to rewire safely
    rig.engine.setModMatrixSource(&matrix);
    rig.engine.prepare(48000.0, 512);

    // Square LFO drives position 0 -> 0.5 half the time; without the mod the
    // engine (position 0, jitter 0) never leaves the start of the source.
    const float maxFrac = rig.maxSrcFractionOver(700, &matrix);
    rig.engine.reset();
    REQUIRE(maxFrac > 0.4f);
}

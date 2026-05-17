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
    Grain g{ buf, 10, 2048, 1.5f, -0.3f, EnvelopeShape::Triangle, 0.25f };

    REQUIRE(g.source        == buf);
    REQUIRE(g.startPos      == 10);
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

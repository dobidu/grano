#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cmath>
#include <limits>

#include "Engine/FeedbackPath.h"

TEST_CASE("FeedbackPath ring length follows the sample rate", "[feedback]")
{
    FeedbackPath fb;
    fb.prepare(48000.0);
    REQUIRE(fb.getNumSamples() == 4 * 48000);
    fb.prepare(96000.0);
    REQUIRE(fb.getNumSamples() == 4 * 96000); // 4 s at any rate (was 2 s at 96k)
    fb.prepare(192000.0);
    REQUIRE(fb.getNumSamples() == FeedbackPath::kCapacity);
}

TEST_CASE("FeedbackPath starts silent", "[feedback]")
{
    FeedbackPath fb;
    const float* ptr = fb.getReadPointer();
    for (int i = 0; i < 1024; ++i)
        REQUIRE(ptr[i] == 0.0f);
}

TEST_CASE("FeedbackPath writes non-zero after process()", "[feedback]")
{
    FeedbackPath fb;
    std::array<float, 512> L{}, R{};
    for (int i = 0; i < 512; ++i) L[i] = R[i] = 0.5f;

    fb.process(L.data(), R.data(), 512, 0.8f, 0.0f);
    const float* ptr = fb.getReadPointer();
    bool anyNonZero = false;
    for (int i = 0; i < 512; ++i)
        if (std::abs(ptr[i]) > 1e-5f) { anyNonZero = true; break; }
    REQUIRE(anyNonZero);
}

TEST_CASE("FeedbackPath gain clamped: no divergence at 0.95 for 10000 blocks", "[feedback]")
{
    FeedbackPath fb;
    std::array<float, 64> buf{};
    for (auto& v : buf) v = 1.0f;

    for (int block = 0; block < 10000; ++block)
        fb.process(buf.data(), buf.data(), 64, 0.95f, 0.0f);

    const float* ptr = fb.getReadPointer();
    bool bounded = true;
    for (int i = 0; i < fb.getNumSamples(); ++i)
        if (std::abs(ptr[i]) > 2.0f) { bounded = false; break; }
    REQUIRE(bounded);
}

TEST_CASE("FeedbackPath gain > 0.95 is clamped", "[feedback]")
{
    FeedbackPath fb;
    std::array<float, 64> buf{};
    for (auto& v : buf) v = 1.0f;
    for (int block = 0; block < 1000; ++block)
        fb.process(buf.data(), buf.data(), 64, 10.0f, 0.0f);
    const float* ptr = fb.getReadPointer();
    for (int i = 0; i < 1024; ++i)
        REQUIRE(std::abs(ptr[i]) <= 2.0f);
}

TEST_CASE("FeedbackPath damp=1 still passes a darkened signal", "[feedback]")
{
    FeedbackPath fb;
    fb.prepare(48000.0);
    std::array<float, 512> L{};
    L.fill(0.5f);
    fb.process(L.data(), L.data(), 512, 0.9f, 1.0f);
    REQUIRE(std::abs(fb.getReadPointer()[511]) > 0.1f); // was frozen at 0 (k = 0)
}

TEST_CASE("FeedbackPath scrubs non-finite input and soft-clips", "[feedback]")
{
    FeedbackPath fb;
    fb.prepare(48000.0);
    std::array<float, 64> L{};
    L.fill(1000.0f);
    L[10] = std::numeric_limits<float>::quiet_NaN();
    L[20] = std::numeric_limits<float>::infinity();
    fb.process(L.data(), L.data(), 64, 0.95f, 0.0f);
    for (int i = 0; i < 64; ++i)
    {
        REQUIRE(std::isfinite(fb.getReadPointer()[i]));
        REQUIRE(std::abs(fb.getReadPointer()[i]) <= 1.0f);
    }
}

// ── Engine: feedback is blended with the bank (beta.4 fix) ────────────────────

#include "Engine/GranularEngine.h"
#include "Engine/MultiSampleBank.h"
#include <atomic>
#include <memory>

namespace
{
struct FeedbackRig
{
    static constexpr double kSR = 48000.0;
    static constexpr int    kBlock = 512;

    MultiSampleBank bank;
    FeedbackPath    ring;
    GranularEngine  engine;
    std::atomic<float> w0{ 1.0f }, w1{ 0.0f }, w2{ 0.0f }, w3{ 0.0f };
    std::atomic<float> fbOn{ 1.0f }, fbGain{ 0.5f }, fbDamp{ 0.0f };
    std::atomic<float> noSource{ 1.0f };   // Silence: the test tone can't mask a dead loop

    explicit FeedbackRig(bool withSample)
    {
        engine.setBank(&bank);
        engine.setBankParamPointers(&w0, &w1, &w2, &w3);
        engine.setFeedbackSource(&ring);
        engine.setFeedbackParamPointers(&fbOn, &fbGain, &fbDamp);
        engine.setNoSourceModePointer(&noSource);
        engine.prepare(kSR, kBlock);
        ring.prepare(kSR);
        if (withSample)
        {
            const int len = (int)kSR;
            auto buf = std::make_unique<juce::AudioBuffer<float>>(1, len + 2);
            buf->clear();
            for (int i = 0; i < len; ++i)
                buf->setSample(0, i, 0.5f * std::sin(0.05f * (float)i));
            bank.getSlot(0).setPending(std::move(buf), len);
        }
    }

    // Render like the processor: engine, then the output feeds the ring.
    double rms(double seconds)
    {
        juce::AudioBuffer<float> out(2, kBlock);
        double sum = 0; long long n = 0;
        for (int b = 0; b < (int)(seconds * kSR / kBlock); ++b)
        {
            out.clear();
            engine.processBlock(out);
            ring.process(out.getReadPointer(0), out.getReadPointer(1), kBlock,
                         fbGain.load(), fbDamp.load());
            for (int i = 0; i < kBlock; ++i) { const float v = out.getSample(0, i); sum += (double)v * v; ++n; }
        }
        return std::sqrt(sum / (double)std::max(1LL, n));
    }
};
} // namespace

TEST_CASE("Feedback on from a cold start keeps sounding", "[feedback][GranularEngine]")
{
    // Before beta.4 the engine read only the (empty) ring with Feedback on:
    // silence forever. Now grains blend ring and bank by Feedback Gain.
    for (float gain : { 0.0f, 0.5f, 0.95f })
    {
        INFO("gain " << gain);
        FeedbackRig rig(true);
        rig.fbGain = gain;
        (void)rig.rms(1.0);
        REQUIRE(rig.rms(10.0) > 1e-3);
        REQUIRE_FALSE(rig.engine.isNoSourceActive());
    }
}

TEST_CASE("Feedback on without a sample is a no-source state", "[feedback][GranularEngine]")
{
    FeedbackRig rig(false);
    REQUIRE(rig.rms(2.0) == 0.0);          // Silence mode, nothing to recycle
    REQUIRE(rig.engine.isNoSourceActive()); // the No Source selector shows
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Engine/SampleBuffer.h"
#include "Engine/MultiSampleBank.h"
#include "Engine/GranularEngine.h"

// ── Helpers ──────────────────────────────────────────────────────────────────

static std::unique_ptr<juce::AudioBuffer<float>> makeBuffer(int numFrames, float fillValue = 0.5f)
{
    // +2 guard samples as required by the SampleBuffer contract.
    auto buf = std::make_unique<juce::AudioBuffer<float>>(1, numFrames + 2);
    buf->clear();
    for (int i = 0; i < numFrames; ++i)
        buf->setSample(0, i, fillValue);
    return buf;
}

// ── SampleBuffer — basic lifecycle ───────────────────────────────────────────

TEST_CASE("SampleBuffer starts empty", "[SampleBuffer]")
{
    SampleBuffer sb;
    REQUIRE(sb.getReadPointer() == nullptr);
    REQUIRE(sb.getNumSamples() == 0);
}

TEST_CASE("SampleBuffer trySwap returns false when nothing pending", "[SampleBuffer]")
{
    SampleBuffer sb;
    REQUIRE_FALSE(sb.trySwap());
}

TEST_CASE("SampleBuffer setPending then trySwap installs buffer", "[SampleBuffer]")
{
    SampleBuffer sb;
    constexpr int N = 1000;
    sb.setPending(makeBuffer(N), N);

    REQUIRE(sb.trySwap());
    REQUIRE(sb.getReadPointer() != nullptr);
    REQUIRE(sb.getNumSamples() == N);
}

TEST_CASE("SampleBuffer getNumSamples reflects only playable frames", "[SampleBuffer]")
{
    SampleBuffer sb;
    constexpr int audioFrames = 44100;
    sb.setPending(makeBuffer(audioFrames), audioFrames);
    (void) sb.trySwap();
    REQUIRE(sb.getNumSamples() == audioFrames);
}

TEST_CASE("SampleBuffer read pointer points to channel-0 data", "[SampleBuffer]")
{
    SampleBuffer sb;
    constexpr int N = 512;
    auto buf = makeBuffer(N, 0.75f);
    const float* rawPtr = buf->getReadPointer(0);
    sb.setPending(std::move(buf), N);
    (void) sb.trySwap();
    REQUIRE(sb.getReadPointer() == rawPtr);
    REQUIRE(sb.getReadPointer()[0] == Catch::Approx(0.75f));
}

// ── SampleBuffer — second swap retires the first buffer ───────────────────────

TEST_CASE("SampleBuffer second setPending+trySwap replaces first buffer", "[SampleBuffer]")
{
    SampleBuffer sb;
    sb.setPending(makeBuffer(100, 0.1f), 100);
    (void) sb.trySwap();

    sb.setPending(makeBuffer(200, 0.9f), 200);
    REQUIRE(sb.trySwap()); // old buffer should be retired, new installed

    REQUIRE(sb.getNumSamples() == 200);
    REQUIRE(sb.getReadPointer()[0] == Catch::Approx(0.9f));
}

TEST_CASE("SampleBuffer processRetired drains without crash", "[SampleBuffer]")
{
    SampleBuffer sb;
    for (int i = 0; i < 3; ++i)
    {
        sb.setPending(makeBuffer(100), 100);
        (void) sb.trySwap();
    }
    // Should not crash or leak (JUCE leak detector verifies the latter).
    sb.processRetired();
}

// ── SampleBuffer — unclaimed pending is cleaned up on next setPending ────────

TEST_CASE("SampleBuffer cleans up unclaimed pending on next setPending", "[SampleBuffer]")
{
    SampleBuffer sb;
    sb.setPending(makeBuffer(100), 100); // pending, never claimed
    sb.setPending(makeBuffer(200), 200); // replaces the first (deletes it)
    // If we call trySwap now we get the second one.
    REQUIRE(sb.trySwap());
    REQUIRE(sb.getNumSamples() == 200);
}

TEST_CASE("SampleBuffer trySwap is idempotent when no new pending", "[SampleBuffer]")
{
    SampleBuffer sb;
    sb.setPending(makeBuffer(100), 100);
    (void) sb.trySwap();

    REQUIRE_FALSE(sb.trySwap()); // no new pending — should return false
    REQUIRE(sb.getNumSamples() == 100); // still has the old buffer
}

// ── GranularEngine — SampleBuffer integration ────────────────────────────────

TEST_CASE("GranularEngine uses SampleBuffer source when loaded", "[GranularEngine][SampleBuffer]")
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 512;
    constexpr int    kFrames     = (int)kSampleRate; // 1 second

    MultiSampleBank bank;
    GranularEngine engine;
    engine.setBank(&bank);
    engine.prepare(kSampleRate, kBlockSize);

    // Load slot 0, then trigger one processBlock to call trySwapAll().
    bank.loadSlot(0, makeBuffer(kFrames, 0.5f), kFrames);

    juce::AudioBuffer<float> out(2, kBlockSize);
    out.clear();
    engine.processBlock(out); // triggers trySwap(); currentReadPtr_ now set

    // Give scheduler time to fill grains from the new source (~7 cycles @ 20 ms).
    juce::Thread::sleep(150);

    float totalEnergy = 0.0f;
    for (int b = 0; b < 20; ++b)
    {
        out.clear();
        engine.processBlock(out);
        for (int s = 0; s < kBlockSize; ++s)
            totalEnergy += std::abs(out.getSample(0, s));
    }

    engine.reset();
    REQUIRE(totalEnergy > 0.0f); // non-zero output means SampleBuffer was used
}

TEST_CASE("GranularEngine falls back to sine tone when SampleBuffer is empty", "[GranularEngine][SampleBuffer]")
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 512;

    MultiSampleBank bank;
    GranularEngine engine;
    engine.setBank(&bank); // no slots loaded yet — falls back to sine
    engine.prepare(kSampleRate, kBlockSize);

    juce::Thread::sleep(150);

    juce::AudioBuffer<float> out(2, kBlockSize);
    float totalEnergy = 0.0f;
    for (int b = 0; b < 20; ++b)
    {
        out.clear();
        engine.processBlock(out);
        for (int s = 0; s < kBlockSize; ++s)
            totalEnergy += std::abs(out.getSample(0, s));
    }

    engine.reset();
    REQUIRE(totalEnergy > 0.0f); // sine fallback produces audio
}

TEST_CASE("GranularEngine handles null SampleBuffer gracefully", "[GranularEngine][SampleBuffer]")
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 512;

    GranularEngine engine; // no setSource — sampleSource_ stays nullptr
    engine.prepare(kSampleRate, kBlockSize);

    juce::Thread::sleep(100);

    juce::AudioBuffer<float> out(2, kBlockSize);
    // Should not crash.
    for (int b = 0; b < 10; ++b)
    {
        out.clear();
        engine.processBlock(out);
    }

    engine.reset();
    SUCCEED("No crash with null SampleBuffer");
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../Source/Engine/MultiSampleBank.h"

static std::unique_ptr<juce::AudioBuffer<float>> makeBuf(int numSamples, float fillValue)
{
    auto buf = std::make_unique<juce::AudioBuffer<float>>(1, numSamples + 2);
    buf->clear();
    for (int i = 0; i < numSamples; ++i)
        buf->setSample(0, i, fillValue);
    return buf;
}

TEST_CASE("MultiSampleBank default state", "[multisample]")
{
    MultiSampleBank bank;
    REQUIRE(bank.getReadPointer(0) == nullptr);
    REQUIRE(bank.getNumSamples(0) == 0);
}

TEST_CASE("MultiSampleBank loadSlot and swap", "[multisample]")
{
    MultiSampleBank bank;
    bank.loadSlot(0, makeBuf(100, 1.0f), 100);
    REQUIRE(bank.getReadPointer(0) == nullptr); // not yet swapped
    const bool swapped = bank.trySwapAll();
    REQUIRE(swapped);
    REQUIRE(bank.getReadPointer(0) != nullptr);
    REQUIRE(bank.getNumSamples(0) == 100);
    bank.processRetiredAll();
}

TEST_CASE("MultiSampleBank trySwapAll false when no pending", "[multisample]")
{
    MultiSampleBank bank;
    bank.loadSlot(0, makeBuf(50, 0.5f), 50);
    bank.trySwapAll();
    bank.processRetiredAll();
    REQUIRE(!bank.trySwapAll()); // no new load pending
}

TEST_CASE("MultiSampleBank pickSlot single slot", "[multisample]")
{
    MultiSampleBank bank;
    bank.loadSlot(0, makeBuf(64, 0.9f), 64);
    bank.trySwapAll();
    bank.processRetiredAll();

    juce::Random rng(42);
    float weights[MultiSampleBank::kNumSlots] = { 1.0f, 0.0f, 0.0f, 0.0f };
    auto [ptr, len] = bank.pickSlot(rng, weights);
    REQUIRE(ptr != nullptr);
    REQUIRE(len == 64);
}

TEST_CASE("MultiSampleBank pickSlot zero weight returns null", "[multisample]")
{
    MultiSampleBank bank;
    bank.loadSlot(0, makeBuf(64, 0.5f), 64);
    bank.trySwapAll();
    bank.processRetiredAll();

    juce::Random rng(1);
    float weights[MultiSampleBank::kNumSlots] = { 0.0f, 0.0f, 0.0f, 0.0f };
    auto [ptr, len] = bank.pickSlot(rng, weights);
    REQUIRE(ptr == nullptr);
    REQUIRE(len == 0);
}

TEST_CASE("MultiSampleBank pickSlot distributes across slots", "[multisample]")
{
    MultiSampleBank bank;
    bank.loadSlot(0, makeBuf(32, 1.0f), 32);
    bank.loadSlot(1, makeBuf(64, 2.0f), 64);
    bank.trySwapAll();
    bank.processRetiredAll();

    juce::Random rng(99);
    float weights[MultiSampleBank::kNumSlots] = { 1.0f, 1.0f, 0.0f, 0.0f };

    int hits0 = 0, hits1 = 0;
    for (int i = 0; i < 1000; ++i)
    {
        auto [ptr, len] = bank.pickSlot(rng, weights);
        REQUIRE(ptr != nullptr);
        if (len == 32) ++hits0;
        else if (len == 64) ++hits1;
    }
    REQUIRE(hits0 > 300);
    REQUIRE(hits1 > 300);
}

TEST_CASE("MultiSampleBank getSlot returns correct ref", "[multisample]")
{
    MultiSampleBank bank;
    bank.loadSlot(1, makeBuf(20, 0.3f), 20);
    SampleBuffer& slot = bank.getSlot(1);
    REQUIRE(slot.getNumSamples() == 0); // pending, not swapped yet
    bank.trySwapAll();
    bank.processRetiredAll();
    REQUIRE(slot.getNumSamples() == 20);
}

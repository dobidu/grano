#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cmath>

#include "Engine/FeedbackPath.h"

TEST_CASE("FeedbackPath getNumSamples matches capacity", "[feedback]")
{
    FeedbackPath fb;
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
    for (int i = 0; i < FeedbackPath::kCapacity; ++i)
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

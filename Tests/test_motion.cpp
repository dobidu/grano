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

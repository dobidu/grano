#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Parameters.h"
#include "PluginProcessor.h"

// ── ParamID strings ──────────────────────────────────────────────────────────

TEST_CASE("ParamIDs are non-empty and unique", "[parameters]")
{
    const char* ids[] = {
        ParamIDs::grainSize,
        ParamIDs::density,
        ParamIDs::position,
        ParamIDs::positionJitter,
        ParamIDs::pitchShift,
        ParamIDs::stereoSpread,
        ParamIDs::masterVolume,
        ParamIDs::loop,
    };
    constexpr int N = (int)(sizeof(ids) / sizeof(ids[0]));
    REQUIRE(N == 8);

    for (int i = 0; i < N; ++i)
    {
        REQUIRE(std::string(ids[i]).size() > 0);
        for (int j = i + 1; j < N; ++j)
            REQUIRE(std::string(ids[i]) != std::string(ids[j]));
    }
}

// ── APVTS registration via GranoAudioProcessor ───────────────────────────────

TEST_CASE("All 8 params registered in APVTS", "[parameters][apvts]")
{
    GranoAudioProcessor proc;
    auto& apvts = proc.getAPVTS();

    REQUIRE(apvts.getParameter(ParamIDs::grainSize)      != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::density)        != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::position)       != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::positionJitter) != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::pitchShift)     != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::stereoSpread)   != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::masterVolume)   != nullptr);
    REQUIRE(apvts.getParameter(ParamIDs::loop)           != nullptr);
}

TEST_CASE("Parameter defaults are in range", "[parameters][defaults]")
{
    GranoAudioProcessor proc;
    auto& apvts = proc.getAPVTS();

    const char* ids[] = {
        ParamIDs::grainSize, ParamIDs::density, ParamIDs::position,
        ParamIDs::positionJitter, ParamIDs::pitchShift, ParamIDs::stereoSpread,
        ParamIDs::masterVolume, ParamIDs::loop,
    };
    for (auto id : ids)
    {
        auto* p = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(id));
        REQUIRE(p != nullptr);
        const float def = p->convertFrom0to1(p->getDefaultValue());
        REQUIRE(def >= p->getNormalisableRange().start);
        REQUIRE(def <= p->getNormalisableRange().end);
    }
}

// ── State round-trip ─────────────────────────────────────────────────────────

TEST_CASE("State serialises and deserialises without loss", "[parameters][state]")
{
    GranoAudioProcessor proc;
    auto& apvts = proc.getAPVTS();

    // Move grainSize away from default so round-trip is detectable.
    if (auto* p = apvts.getParameter(ParamIDs::grainSize))
        p->setValueNotifyingHost(p->getNormalisableRange().convertTo0to1(250.0f));

    const float before = apvts.getRawParameterValue(ParamIDs::grainSize)
        ->load(std::memory_order_relaxed);

    juce::MemoryBlock block;
    proc.getStateInformation(block);
    REQUIRE(block.getSize() > 0);

    // Reset to default, then restore.
    if (auto* p = apvts.getParameter(ParamIDs::grainSize))
        p->setValueNotifyingHost(p->getDefaultValue());

    proc.setStateInformation(block.getData(), (int)block.getSize());

    const float after = apvts.getRawParameterValue(ParamIDs::grainSize)
        ->load(std::memory_order_relaxed);

    REQUIRE(after == Catch::Approx(before).epsilon(0.001f));
}

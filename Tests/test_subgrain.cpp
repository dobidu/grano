#include <catch2/catch_test_macros.hpp>
#include <array>

#include "Engine/SubGrain.h"
#include "Engine/GrainPool.h"

static Grain makeParent()
{
    static float dummy[1024]{};
    Grain g{};
    g.source        = dummy;
    g.startPos      = 100;
    g.lengthSamples = 512;
    g.pitchRatio    = 1.0f;
    g.pan           = 0.0f;
    g.shape         = EnvelopeShape::Hann;
    g.currentPhase  = 0.0f;
    g.reversed      = false;
    return g;
}

TEST_CASE("SubGrain depth 0 is no-op", "[subgrain]")
{
    GrainPool pool;
    std::array<Grain*, 32> slots{};
    juce::Random rng(42);
    const Grain parent = makeParent();

    const int written = SubGrain::spawnInto(parent, 0, pool, slots.data(), 0, 32, rng);
    REQUIRE(written == 0);
}

TEST_CASE("SubGrain depth 1 spawns kMinSubs..kMaxSubs grains", "[subgrain]")
{
    for (int seed = 0; seed < 20; ++seed)
    {
        GrainPool pool;
        std::array<Grain*, 64> slots{};
        juce::Random rng(seed);
        const Grain parent = makeParent();

        const int written = SubGrain::spawnInto(parent, 1, pool, slots.data(), 0, 64, rng);
        REQUIRE(written >= SubGrain::kMinSubs);
        REQUIRE(written <= SubGrain::kMaxSubs);

        for (int i = 0; i < written; ++i)
        {
            REQUIRE(slots[i] != nullptr);
            // Sub-grain shorter than or equal to half parent
            REQUIRE(slots[i]->lengthSamples <= parent.lengthSamples / 2);
            // Sub-grain starts within parent window
            REQUIRE(slots[i]->startPos >= parent.startPos);
            REQUIRE(slots[i]->startPos < parent.startPos + parent.lengthSamples);
        }
    }
}

TEST_CASE("SubGrain depth 2 spawns within expected bounds", "[subgrain]")
{
    constexpr int maxPossible = SubGrain::kMaxSubs * (1 + SubGrain::kMaxSubs2);

    for (int seed = 0; seed < 20; ++seed)
    {
        GrainPool pool;
        std::array<Grain*, 64> slots{};
        juce::Random rng(seed);
        const Grain parent = makeParent();

        const int written = SubGrain::spawnInto(parent, 2, pool, slots.data(), 0, 64, rng);
        REQUIRE(written >= SubGrain::kMinSubs); // at least depth-1 subs
        REQUIRE(written <= maxPossible);

        for (int i = 0; i < written; ++i)
            REQUIRE(slots[i] != nullptr);
    }
}

TEST_CASE("SubGrain writeEnd cap: no write past slot limit", "[subgrain]")
{
    GrainPool pool;
    // Give only 2 slots — even depth 2 must not exceed this
    std::array<Grain*, 4> slots{};
    juce::Random rng(99);
    const Grain parent = makeParent();

    const int written = SubGrain::spawnInto(parent, 2, pool, slots.data(), 0, 2, rng);
    REQUIRE(written <= 2);
}

#pragma once
#include "Grain.h"
#include "GrainPool.h"
#include <juce_core/juce_core.h>

// SubGrain — spawns overlapping sub-grains within a parent grain window.
// Called from the scheduler thread only (not audio thread). No allocation.
//
// depth 0: no-op.
// depth 1: spawns kMinSubs..kMaxSubs sub-grains inside parent window.
// depth 2: recurses once — each depth-1 sub spawns kMinSubs2..kMaxSubs2 sub-sub-grains.
//
// Returns number of extra grains written into fifoSlots[writeStart..writeStart+return].
struct SubGrain
{
    static constexpr int kMinSubs  = 2;
    static constexpr int kMaxSubs  = 4;
    static constexpr int kMinSubs2 = 2;
    static constexpr int kMaxSubs2 = 3;

    // Spawns sub-grains for `parent` at `depth` into `pool`.
    // Extra grains are written into fifoSlots[writeStart .. writeStart+n-1].
    // writeEnd is the exclusive upper bound (fifoSlots must be at least writeEnd elements).
    // Returns count of extra grains written. RT-safe: no allocation, no locks.
    static int spawnInto(const Grain& parent,
                         int depth,
                         GrainPool& pool,
                         Grain** fifoSlots,
                         int writeStart,
                         int writeEnd,
                         juce::Random& rng) noexcept;
};

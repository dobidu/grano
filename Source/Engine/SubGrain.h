#pragma once
#include "Grain.h"
#include "GrainPool.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>

// SubGrain — spawns overlapping sub-grains within a parent grain window.
// Called from the audio thread (sample-clock scheduler). No allocation.
//
// depth 0: no-op.
// depth 1: spawns kMinSubs..kMaxSubs sub-grains inside parent window.
// depth 2: recurses once — each depth-1 sub spawns kMinSubs2..kMaxSubs2 sub-sub-grains.
//
// Each sub is placed in TIME inside the parent window (startDelay = parent
// delay + offset) and reads the source region the parent reads at that
// moment, so the family stays coherent with the parent grain.
//
// Returns number of extra grains written into out[writeStart..writeStart+return).
struct SubGrain
{
    static constexpr int kMinSubs  = 2;
    static constexpr int kMaxSubs  = 4;
    static constexpr int kMinSubs2 = 2;
    static constexpr int kMaxSubs2 = 3;
    static constexpr int kMaxFamily = kMaxSubs * (1 + kMaxSubs2);

    // out must be at least writeEnd elements. RT-safe: no allocation, no locks.
    static int spawnInto(const Grain& parent,
                         int depth,
                         GrainPool& pool,
                         Grain** out,
                         int writeStart,
                         int writeEnd,
                         juce::Random& rng) noexcept;
};

// Constrain a grain so its whole read span lies inside its source:
// reads cover [startPos, startPos + lengthSamples * pitchRatio] (linear
// interpolation also touches index+1, covered by the +2 guard samples).
// A grain that would run off the end is pulled back rather than truncated,
// so its envelope always completes (R-03). Ring sources (wrapLen > 0) are
// exempt — they are positioned by the engine relative to the write head.
inline void fitGrainToSource(Grain& g) noexcept
{
    if (g.wrapLen > 0 || g.sourceLen <= 1 || g.pitchRatio <= 0.0f)
        return;
    const double maxSpan = (double)(g.sourceLen - 1);
    double span = (double)g.lengthSamples * (double)g.pitchRatio;
    if (span > maxSpan)
    {
        g.lengthSamples = std::max(1, (int)(maxSpan / (double)g.pitchRatio));
        span = (double)g.lengthSamples * (double)g.pitchRatio;
    }
    const int maxStart = std::max(0, (int)std::floor(maxSpan - span));
    g.startPos = std::clamp(g.startPos, 0, maxStart);
}

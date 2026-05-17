#pragma once

#include "Grain.h"
#include <array>
#include <atomic>

// GrainPool — pre-allocated grain slot manager.
//
// Owns 1024 Grain objects in a fixed std::array (no heap after construction).
// acquire() and release() are RT-safe: bounded scan, CAS atomic, no locks,
// no allocation. Safe to call from the audio thread.
//
// Threading contract:
//   acquire() — called from worker threads (grain scheduler).
//   release() — called from audio thread when grain finishes playback.
//   Both paths are lock-free; multiple concurrent acquire() callers are safe.
class GrainPool
{
public:
    static constexpr int Capacity = 1024;

    GrainPool() noexcept;

    // Acquire a free slot. Returns nullptr if pool is exhausted.
    // RT-safe: O(Capacity) worst case, O(1) typical via hint_.
    [[nodiscard]] Grain* acquire() noexcept;

    // Return a slot to the pool. ptr must have been returned by acquire().
    // RT-safe: single atomic store.
    void release(Grain* grain) noexcept;

    // Count currently active slots (approximate; uses relaxed loads).
    int activeCount() const noexcept;

private:
    std::array<Grain, Capacity>             slots_;
    std::array<std::atomic<bool>, Capacity> inUse_;
    std::atomic<int>                        hint_{ 0 };
};

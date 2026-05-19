#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>

// FeedbackPath — pre-allocated ring buffer filled by the audio thread.
// Provides a grain source of the engine's own recent audio output.
// RT-safe: process() has no allocation, no locks. Scheduler thread
// reads via getReadPointer()/getNumSamples() (minor racy reads accepted,
// same policy as GrainSnapshot — aligned float, x86 atomic).
class FeedbackPath
{
public:
    static constexpr int kCapacity = 48000 * 4; // 4 s @ 48 kHz

    FeedbackPath() noexcept = default;

    // Called from audio thread after engine::processBlock.
    // Mixes L+R to mono, applies LP filter (damp), multiplies by gain,
    // writes to ring buffer. gain clamped to [0, 0.95].
    void process(const float* left, const float* right,
                 int numSamples, float gain, float damp) noexcept;

    // Called from scheduler thread. Returns ring buffer base pointer.
    // startPos for grains is chosen relative to writePos_ for recency.
    const float* getReadPointer() const noexcept { return buffer_.data(); }
    int          getNumSamples()  const noexcept { return kCapacity; }

    // Returns the current write position (approx. "most recent" sample index).
    int writePos() const noexcept { return writePos_.load(std::memory_order_relaxed); }

private:
    alignas(64) std::array<float, kCapacity> buffer_{};
    std::atomic<int> writePos_{ 0 };
    float            lpState_{ 0.0f };
};

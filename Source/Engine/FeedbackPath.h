#pragma once
#include <atomic>
#include <cmath>
#include <algorithm>
#include <vector>

// FeedbackPath — pre-allocated ring buffer of the plugin's own recent output,
// used as a grain source (internal feedback).
//
// Threading: process(), reset() and all grain reads happen on the audio
// thread (the engine schedules on the sample clock, 08-08). prepare() runs
// on the message thread with audio stopped.
//
// RT-safe: process() has no allocation, no locks. Storage is allocated once
// in the constructor for the largest supported sample rate.
class FeedbackPath
{
public:
    static constexpr double kSeconds       = 4.0;
    static constexpr int    kMaxSampleRate = 192000;
    static constexpr int    kCapacity      = (int)(kSeconds * kMaxSampleRate);

    FeedbackPath() : buffer_((size_t)kCapacity + 2, 0.0f) {}

    // Message thread, audio stopped: size the active ring to kSeconds at the
    // session rate and clear it (R-25).
    void prepare(double sampleRate) noexcept
    {
        activeLen_ = std::clamp((int)(kSeconds * sampleRate), 1024, kCapacity);
        reset();
    }

    // Mixes L+R to mono, one-pole low-pass (damp), gain, soft-clip, then
    // writes to the ring. gain is clamped to [0, 0.95]; damp=1 still passes
    // a darkened signal (k floor 0.02, R-13). tanh bounds every stored
    // sample to |x| < 1 so the loop can't run away, and non-finite input is
    // scrubbed so a NaN can never latch in the ring (R-11).
    void process(const float* left, const float* right,
                 int numSamples, float gain, float damp) noexcept;

    // Audio thread, on feedback re-enable: clear stale ring content so old
    // audio doesn't burst back in. Plain fill — no allocation or locks.
    void reset() noexcept
    {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        writePos_.store(0, std::memory_order_relaxed);
        lpState_ = 0.0f;
    }

    // Ring base pointer. getNumSamples() is the active ring length; the
    // +2 zero guard samples past it keep the renderGrain interpolation
    // contract for non-wrapping readers.
    const float* getReadPointer() const noexcept { return buffer_.data(); }
    int          getNumSamples()  const noexcept { return activeLen_; }

    // Next index to be written (everything before it is valid audio).
    int writePos() const noexcept { return writePos_.load(std::memory_order_relaxed); }

private:
    std::vector<float> buffer_;
    int                activeLen_{ (int)(kSeconds * 48000) };
    std::atomic<int>   writePos_{ 0 };
    float              lpState_{ 0.0f };
};

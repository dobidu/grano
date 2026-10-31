#pragma once
#include "SampleBuffer.h"
#include <juce_core/juce_core.h>
#include <array>
#include <utility>

// MultiSampleBank — 4-slot SampleBuffer wrapper with per-grain weighted selection.
//
// Load contract (mirrors SampleBuffer):
//   loadSlot()          — message/loading thread
//   trySwapAll()        — audio thread, once per processBlock; returns true if any slot swapped
//   processRetiredAll() — message thread (~30 Hz Timer)
//   pickSlot()          — audio thread (RT-safe: stack arrays only, no alloc, no locks)
class MultiSampleBank
{
public:
    static constexpr int kNumSlots = 4;

    MultiSampleBank() = default;

    // Message/loading thread: load a buffer into a slot.
    // Returns the slot's load generation for this buffer (0 if slot invalid).
    juce::uint32 loadSlot(int slot,
                          std::unique_ptr<juce::AudioBuffer<float>> buf,
                          int numSamples);

    // Any thread: generation of the buffer currently live in `slot` (0 = none).
    juce::uint32 getCurrentGeneration(int slot) const noexcept
    {
        return (slot >= 0 && slot < kNumSlots) ? slots_[slot].getCurrentGeneration() : 0;
    }

    // Audio thread: try-swap all pending buffers.
    // Returns true if any slot swapped (caller should flush active grains).
    [[nodiscard]] bool trySwapAll() noexcept;

    // Message thread (~30 Hz): delete retired buffers for all slots.
    void processRetiredAll();

    // Audio thread (sample-clock scheduler): weighted-random slot selection. RT-safe.
    // weights[kNumSlots] — unnormalized; slots with weight=0 or no data excluded.
    // Returns {ptr, len} for the selected slot, or {nullptr, 0} if none available.
    std::pair<const float*, int> pickSlot(juce::Random& rng,
                                          const float weights[kNumSlots]) const noexcept;

    // Direct per-slot read access (any thread, same safety as SampleBuffer).
    const float* getReadPointer(int slot) const noexcept;
    int          getNumSamples(int slot)  const noexcept;

    // Reference to a specific slot's SampleBuffer (e.g. slot 0 for WaveformDisplay).
    SampleBuffer& getSlot(int slot) noexcept { return slots_[slot]; }

private:
    std::array<SampleBuffer, kNumSlots> slots_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiSampleBank)
};

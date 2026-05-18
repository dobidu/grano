#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>
#include <memory>

// SampleBuffer — RT-safe hot-swappable audio buffer.
//
// Manages three buffer states:
//   pending  — loading thread writes, audio thread claims
//   current  — audio thread owns exclusively (live read source)
//   retired  — audio thread deposits old buffers, message thread deletes
//
// Threading contract:
//   setPending()     — message/loading thread only
//   trySwap()        — audio thread only, once per processBlock
//   processRetired() — message thread only (~30 Hz via Timer)
//   getReadPointer() — any thread, safe after trySwap()
//   getNumSamples()  — any thread, safe after trySwap()
//
// Caller must allocate buffers with 2 extra guard samples at the end
// (indices [numAudioSamples] and [numAudioSamples+1] = 0.0f) to prevent
// linear-interpolation out-of-bounds reads at phase = 1.0.
class SampleBuffer
{
public:
    SampleBuffer() = default;
    ~SampleBuffer();

    // Loading thread: transfer ownership of a new buffer.
    // numAudioSamples = actual playable frames (excludes 2 guard samples).
    // If a previous pending buffer was never claimed it is discarded here.
    void setPending(std::unique_ptr<juce::AudioBuffer<float>> newBuf,
                    int numAudioSamples);

    // Audio thread: atomically claim pending buffer, retire old current.
    // Returns true if a new buffer was installed. noexcept, zero allocation.
    [[nodiscard]] bool trySwap() noexcept;

    // Audio thread: read pointer to channel-0 data. nullptr if no buffer loaded.
    const float* getReadPointer() const noexcept;

    // Audio thread: playable sample count (excludes guard samples). 0 if empty.
    int getNumSamples() const noexcept;

    // Message thread (Timer at ~30 Hz): delete any retired buffers.
    void processRetired();

private:
    // Audio thread owns current exclusively.
    std::unique_ptr<juce::AudioBuffer<float>> current_;

    // Read-optimised atomics — safe to read from any thread.
    std::atomic<const float*> currentReadPtr_   { nullptr };
    std::atomic<int>          currentNumSamples_{ 0 };

    // Loading thread writes here; audio thread claims via CAS.
    std::atomic<juce::AudioBuffer<float>*> pending_           { nullptr };
    std::atomic<int>                       pendingNumSamples_ { 0 };

    // Retired buffers waiting for message-thread deletion.
    // Capacity 4: handles up to 4 sample loads between processRetired() calls.
    static constexpr int kRetiredCapacity = 4;
    juce::AbstractFifo                                     retiredFifo_{ kRetiredCapacity };
    std::array<juce::AudioBuffer<float>*, kRetiredCapacity> retiredSlots_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBuffer)
};

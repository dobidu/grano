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
    // Returns the load generation of this buffer (monotonic per slot, > 0).
    juce::uint32 setPending(std::unique_ptr<juce::AudioBuffer<float>> newBuf,
                            int numAudioSamples);

    // Audio thread: atomically claim pending buffer, retire old current.
    // Returns true if a new buffer was installed. noexcept, zero allocation,
    // never frees memory: if the retire queue is full the swap is deferred
    // to a later block (R-21).
    [[nodiscard]] bool trySwap() noexcept;

    // Audio thread: read pointer to channel-0 data. nullptr if no buffer loaded.
    const float* getReadPointer() const noexcept;

    // Audio thread: playable sample count (excludes guard samples). 0 if empty.
    int getNumSamples() const noexcept;

    // Any thread: generation of the buffer currently installed (0 = none).
    juce::uint32 getCurrentGeneration() const noexcept
    {
        return currentGen_.load(std::memory_order_acquire);
    }

    // Message thread (Timer at ~30 Hz): delete any retired buffers.
    void processRetired();

private:
    // A buffer travels with its own length + generation so the audio thread
    // can never pair one load's data with another load's length (R-06).
    // The same record carries the OLD buffer to the retire queue, so the
    // audio thread neither allocates nor frees.
    struct Record
    {
        std::unique_ptr<juce::AudioBuffer<float>> buffer;
        int          numSamples = 0;
        juce::uint32 generation = 0;
    };

    // Audio thread owns current exclusively.
    std::unique_ptr<juce::AudioBuffer<float>> current_;

    // Read-optimised atomics — safe to read from any thread.
    std::atomic<const float*>  currentReadPtr_   { nullptr };
    std::atomic<int>           currentNumSamples_{ 0 };
    std::atomic<juce::uint32>  currentGen_       { 0 };

    // Loading thread writes here; audio thread claims via CAS.
    std::atomic<Record*> pending_{ nullptr };
    juce::uint32         nextGen_{ 0 }; // loading thread only

    // Retired records waiting for message-thread deletion.
    static constexpr int kRetiredCapacity = 16;
    juce::AbstractFifo                          retiredFifo_{ kRetiredCapacity };
    std::array<Record*, kRetiredCapacity>       retiredSlots_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBuffer)
};

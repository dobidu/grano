#include "SampleBuffer.h"

SampleBuffer::~SampleBuffer()
{
    // Delete any unclaimed pending buffer.
    auto* p = pending_.exchange(nullptr, std::memory_order_acquire);
    delete p;

    // Delete remaining retired buffers.
    processRetired();
}

void SampleBuffer::setPending(std::unique_ptr<juce::AudioBuffer<float>> newBuf,
                               int numAudioSamples)
{
    pendingNumSamples_.store(numAudioSamples, std::memory_order_relaxed);

    auto* raw = pending_.exchange(newBuf.release(), std::memory_order_acq_rel);
    delete raw; // Delete any previously pending buffer that was never claimed.
}

bool SampleBuffer::trySwap() noexcept
{
    auto* incoming = pending_.load(std::memory_order_acquire);
    if (incoming == nullptr)
        return false;

    // CAS: claim the pending buffer.
    if (!pending_.compare_exchange_strong(incoming, nullptr,
                                          std::memory_order_acq_rel,
                                          std::memory_order_relaxed))
        return false;

    // Read the sample count for the incoming buffer before retiring current.
    const int newCount = pendingNumSamples_.load(std::memory_order_relaxed);

    // Retire the old current buffer via the FIFO for message-thread deletion.
    if (current_ != nullptr)
    {
        int s1{}, c1{}, s2{}, c2{};
        retiredFifo_.prepareToWrite(1, s1, c1, s2, c2);
        if (c1 > 0)
        {
            retiredSlots_[static_cast<std::size_t>(s1)] = current_.release();
            retiredFifo_.finishedWrite(1);
        }
        else
        {
            // FIFO full — delete immediately to prevent unbounded growth.
            // Rare: requires 4 loads between processRetired() calls (~30 Hz).
            current_.reset();
        }
    }

    // Install new buffer.
    current_.reset(incoming);
    currentReadPtr_.store(current_->getReadPointer(0), std::memory_order_release);
    currentNumSamples_.store(newCount, std::memory_order_release);

    return true;
}

const float* SampleBuffer::getReadPointer() const noexcept
{
    return currentReadPtr_.load(std::memory_order_acquire);
}

int SampleBuffer::getNumSamples() const noexcept
{
    return currentNumSamples_.load(std::memory_order_acquire);
}

void SampleBuffer::processRetired()
{
    const int numReady = retiredFifo_.getNumReady();
    if (numReady == 0)
        return;

    int s1{}, c1{}, s2{}, c2{};
    retiredFifo_.prepareToRead(numReady, s1, c1, s2, c2);
    for (int i = 0; i < c1; ++i)
        delete retiredSlots_[static_cast<std::size_t>(s1 + i)];
    for (int i = 0; i < c2; ++i)
        delete retiredSlots_[static_cast<std::size_t>(s2 + i)];
    retiredFifo_.finishedRead(c1 + c2);
}

#include "SampleBuffer.h"

SampleBuffer::~SampleBuffer()
{
    delete pending_.exchange(nullptr, std::memory_order_acquire);
    processRetired();
}

juce::uint32 SampleBuffer::setPending(std::unique_ptr<juce::AudioBuffer<float>> newBuf,
                                      int numAudioSamples)
{
    auto* rec = new Record{ std::move(newBuf), numAudioSamples, ++nextGen_ };
    const auto gen = rec->generation;
    delete pending_.exchange(rec, std::memory_order_acq_rel); // drop unclaimed older load
    return gen;
}

bool SampleBuffer::trySwap() noexcept
{
    if (pending_.load(std::memory_order_acquire) == nullptr)
        return false;

    // Only this thread writes the retire queue, so free space can only grow
    // concurrently — checking first guarantees the push below succeeds.
    if (retiredFifo_.getFreeSpace() < 1)
        return false; // defer; the message thread will drain the queue

    Record* rec = pending_.exchange(nullptr, std::memory_order_acq_rel);
    if (rec == nullptr)
        return false;

    // Swap buffers: rec now carries the OLD buffer (possibly null) away.
    std::swap(current_, rec->buffer);
    const int          newCount = rec->numSamples;
    const juce::uint32 newGen   = rec->generation;

    int s1{}, c1{}, s2{}, c2{};
    retiredFifo_.prepareToWrite(1, s1, c1, s2, c2);
    retiredSlots_[static_cast<std::size_t>(c1 > 0 ? s1 : s2)] = rec;
    retiredFifo_.finishedWrite(1);

    currentReadPtr_.store(current_ ? current_->getReadPointer(0) : nullptr,
                          std::memory_order_release);
    currentNumSamples_.store(current_ ? newCount : 0, std::memory_order_release);
    currentGen_.store(newGen, std::memory_order_release);
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

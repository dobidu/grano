#include "MultiSampleBank.h"

void MultiSampleBank::loadSlot(int slot,
                                std::unique_ptr<juce::AudioBuffer<float>> buf,
                                int numSamples)
{
    if (slot < 0 || slot >= kNumSlots)
        return;
    slots_[slot].setPending(std::move(buf), numSamples);
}

bool MultiSampleBank::trySwapAll() noexcept
{
    bool anySwapped = false;
    for (auto& slot : slots_)
        if (slot.trySwap())
            anySwapped = true;
    return anySwapped;
}

void MultiSampleBank::processRetiredAll()
{
    for (auto& slot : slots_)
        slot.processRetired();
}

std::pair<const float*, int> MultiSampleBank::pickSlot(
    juce::Random& rng,
    const float weights[kNumSlots]) const noexcept
{
    // Build cumulative weight table over valid (loaded + weight>0) slots.
    float cumulative[kNumSlots];
    int   validIdx[kNumSlots];
    float total = 0.f;
    int   count = 0;

    for (int s = 0; s < kNumSlots; ++s)
    {
        const float w   = weights[s];
        const int   len = slots_[s].getNumSamples();
        if (w > 0.f && len > 0)
        {
            total += w;
            cumulative[count] = total;
            validIdx[count]   = s;
            ++count;
        }
    }

    if (count == 0 || total <= 0.f)
        return { nullptr, 0 };

    const float pick = rng.nextFloat() * total;
    for (int i = 0; i < count; ++i)
    {
        if (pick <= cumulative[i] || i == count - 1)
        {
            const int s = validIdx[i];
            return { slots_[s].getReadPointer(), slots_[s].getNumSamples() };
        }
    }
    return { nullptr, 0 };
}

const float* MultiSampleBank::getReadPointer(int slot) const noexcept
{
    if (slot < 0 || slot >= kNumSlots) return nullptr;
    return slots_[slot].getReadPointer();
}

int MultiSampleBank::getNumSamples(int slot) const noexcept
{
    if (slot < 0 || slot >= kNumSlots) return 0;
    return slots_[slot].getNumSamples();
}

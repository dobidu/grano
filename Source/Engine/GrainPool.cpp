#include "GrainPool.h"
#include <juce_core/juce_core.h>

GrainPool::GrainPool() noexcept
{
    for (auto& flag : inUse_)
        flag.store(false, std::memory_order_relaxed);
}

Grain* GrainPool::acquire() noexcept
{
    const int start = hint_.load(std::memory_order_relaxed);

    for (int i = 0; i < Capacity; ++i)
    {
        const int idx = (start + i) % Capacity;
        bool expected = false;

        if (inUse_[idx].compare_exchange_strong(expected, true,
                                                std::memory_order_acquire,
                                                std::memory_order_relaxed))
        {
            hint_.store((idx + 1) % Capacity, std::memory_order_relaxed);
            slots_[idx] = Grain{}; // zero-initialise before handing out
            return &slots_[idx];
        }
    }

    return nullptr; // pool exhausted
}

void GrainPool::release(Grain* grain) noexcept
{
    if (grain == nullptr)
        return;

    const auto idx = static_cast<int>(grain - slots_.data());
    jassert(idx >= 0 && idx < Capacity); // compiles out in Release
    inUse_[idx].store(false, std::memory_order_release);
}

int GrainPool::activeCount() const noexcept
{
    int count = 0;
    for (const auto& flag : inUse_)
        count += flag.load(std::memory_order_relaxed) ? 1 : 0;
    return count;
}

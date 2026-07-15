#include "ModMatrix.h"
#include <algorithm>

void ModMatrix::setLfos(Lfo* lfo1, Lfo* lfo2) noexcept
{
    lfo1_ = lfo1;
    lfo2_ = lfo2;
}

void ModMatrix::setSlotParams(int slot,
                               std::atomic<float>* source,
                               std::atomic<float>* dest,
                               std::atomic<float>* amount) noexcept
{
    if (slot < 0 || slot >= kNumSlots) return;
    slots_[slot] = { source, dest, amount };
}

void ModMatrix::prepare(double /*sampleRate*/) noexcept { reset(); }

void ModMatrix::reset() noexcept
{
    for (auto& v : modSums_)
        v.store(0.0f, std::memory_order_relaxed);
}

float ModMatrix::getModOffset(Dest d) const noexcept
{
    if (d <= kNone || d >= kNumDests) return 0.0f;
    // Clamp: up to 8 slots can stack on one destination (raw sum ±8); consumers
    // scale by their full parameter range, so an unclamped sum produces absurd
    // values (e.g. ±192 semitones of pitch — instantly-dying grains).
    return std::clamp(
        modSums_[static_cast<int>(d)].load(std::memory_order_relaxed), -1.0f, 1.0f);
}

void ModMatrix::processBlock(int numSamples, double bpm) noexcept
{
    // Step 1: advance LFO1 (sync tick + one value per block, phase advanced by block size)
    if (lfo1_) lfo1_->processBlock(numSamples, bpm);
    const float lfo1Val = lfo1_ ? lfo1_->advance(numSamples) : 0.0f;

    // Step 2: scan for LFO1→kLfo2Rate cross-mod; apply before LFO2 advances
    float lfo2RateMod = 0.0f;
    for (auto& s : slots_)
    {
        if (!s.pSource || !s.pDest || !s.pAmount) continue;
        const int   src = static_cast<int>(s.pSource->load(std::memory_order_relaxed));
        const int   dst = static_cast<int>(s.pDest->load(std::memory_order_relaxed));
        const float amt = s.pAmount->load(std::memory_order_relaxed);
        if (src == 1 /*LFO1*/ && dst == static_cast<int>(kLfo2Rate))
            lfo2RateMod += lfo1Val * amt * 8000.0f;
    }
    if (lfo2_) lfo2_->setRateModOffset(lfo2RateMod);

    // Step 3: advance LFO2 (now with cross-mod rate offset)
    if (lfo2_) lfo2_->processBlock(numSamples, bpm);
    const float lfo2Val = lfo2_ ? lfo2_->advance(numSamples) : 0.0f;

    // Step 4: accumulate mod sums on the stack (no heap)
    float accum[kNumDests]{};
    for (auto& s : slots_)
    {
        if (!s.pSource || !s.pDest || !s.pAmount) continue;
        const int   src = static_cast<int>(s.pSource->load(std::memory_order_relaxed));
        const int   dst = static_cast<int>(s.pDest->load(std::memory_order_relaxed));
        const float amt = s.pAmount->load(std::memory_order_relaxed);
        if (src == 0 || dst <= static_cast<int>(kNone) || dst >= kNumDests) continue;
        const float lfoVal = (src == 1) ? lfo1Val : lfo2Val;
        accum[dst] += lfoVal * amt;
    }

    // Step 5: store results atomically for scheduler-thread reads
    for (int i = 0; i < kNumDests; ++i)
        modSums_[i].store(accum[i], std::memory_order_relaxed);
}

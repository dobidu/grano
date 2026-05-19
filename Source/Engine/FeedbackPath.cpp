#include "FeedbackPath.h"

void FeedbackPath::process(const float* left, const float* right,
                           int numSamples, float gain, float damp) noexcept
{
    const float safeGain = std::min(gain, 0.95f);
    const float k        = 1.0f - std::clamp(damp, 0.0f, 1.0f);
    int pos = writePos_.load(std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
    {
        const float mono = (left[i] + (right ? right[i] : left[i])) * 0.5f;
        lpState_ = lpState_ * (1.0f - k) + mono * k;
        buffer_[pos] = lpState_ * safeGain;
        if (++pos >= kCapacity)
            pos = 0;
    }
    writePos_.store(pos, std::memory_order_relaxed);
}

#include "FeedbackPath.h"

void FeedbackPath::process(const float* left, const float* right,
                           int numSamples, float gain, float damp) noexcept
{
    const float safeGain = std::clamp(gain, 0.0f, 0.95f);
    const float k        = 1.0f - 0.98f * std::clamp(damp, 0.0f, 1.0f);
    const int   len      = activeLen_;
    int pos = writePos_.load(std::memory_order_relaxed);
    if (pos >= len)
        pos = 0;

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = (left[i] + (right ? right[i] : left[i])) * 0.5f;
        if (!std::isfinite(mono))
            mono = 0.0f;
        lpState_ += k * (mono - lpState_);
        if (!std::isfinite(lpState_))
            lpState_ = 0.0f;
        buffer_[(size_t)pos] = std::tanh(lpState_ * safeGain);
        if (++pos >= len)
            pos = 0;
    }
    writePos_.store(pos, std::memory_order_relaxed);
}

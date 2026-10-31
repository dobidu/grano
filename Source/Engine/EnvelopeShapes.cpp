#include "EnvelopeShapes.h"
#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>

float applyEnvelope(EnvelopeShape shape, float phase) noexcept
{
    // Clamp phase to [0, 1] defensively — callers should not pass out-of-range values.
    const float p = std::clamp(phase, 0.0f, 1.0f);

    switch (shape)
    {
        case EnvelopeShape::Hann:
            return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * p));

        case EnvelopeShape::Tukey:
        {
            // Cosine taper at each end, flat centre. α = 0.5 → 25% taper each side.
            constexpr float alpha    = 0.5f;
            constexpr float halfAlpha = alpha * 0.5f;
            if (p < halfAlpha)
                return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * p / halfAlpha));
            if (p > 1.0f - halfAlpha)
                return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * (1.0f - p) / halfAlpha));
            return 1.0f;
        }

        case EnvelopeShape::Gaussian:
        {
            // σ expressed as a fraction of the half-window.
            // Edge-subtracted and renormalised so the window starts and ends
            // at exactly 0 — the raw bell is 0.044 at the edges, a -27 dB
            // step on every grain onset/offset (R-20).
            constexpr float sigma = 0.4f * 0.5f;
            const float     x     = (p - 0.5f) / sigma;
            const float     edge  = 0.04393693f; // exp(-0.5 * (0.5/sigma)^2)
            const float     g     = std::exp(-0.5f * x * x);
            return std::clamp((g - edge) / (1.0f - edge), 0.0f, 1.0f);
        }

        case EnvelopeShape::Triangle:
            return p < 0.5f ? 2.0f * p : 2.0f * (1.0f - p);

        case EnvelopeShape::Rectangle:
            return 1.0f;
    }

    return 0.0f; // unreachable — silences compiler warning
}

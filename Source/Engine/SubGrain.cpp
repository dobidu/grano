#include "SubGrain.h"
#include <algorithm>
#include <cmath>

static Grain* spawnOne(const Grain& parent, int numSubs,
                       GrainPool& pool, juce::Random& rng) noexcept
{
    Grain* g = pool.acquire();
    if (g == nullptr)
        return nullptr;

    const int subLen    = std::max(1, parent.lengthSamples / numSubs);
    const int maxOffset = std::max(0, parent.lengthSamples - subLen);
    const int offset    = (int)(rng.nextFloat() * (float)maxOffset); // output samples

    // Source position the parent reads at time `offset` (forward), or the
    // region it will have reached by the end of the sub (reversed).
    const double p = (double)parent.pitchRatio;
    const int srcOffset = parent.reversed
        ? (int)((double)(parent.lengthSamples - offset - subLen) * p)
        : (int)((double)offset * p);

    *g = Grain{};
    g->source        = parent.source;
    g->sourceLen     = parent.sourceLen;
    g->wrapLen       = parent.wrapLen;
    g->startPos      = parent.startPos + std::max(0, srcOffset);
    if (g->wrapLen > 0)
        g->startPos %= g->wrapLen;
    g->lengthSamples = subLen;
    g->pitchRatio    = parent.pitchRatio * (0.9f + rng.nextFloat() * 0.2f);
    g->pan           = juce::jlimit(-1.0f, 1.0f,
                           parent.pan + (rng.nextFloat() - 0.5f) * 0.4f);
    g->shape         = parent.shape;
    g->elapsed       = 0;
    g->reversed      = parent.reversed;
    g->startDelay    = parent.startDelay + offset;
    g->gain          = parent.gain;
    fitGrainToSource(*g);
    return g;
}

int SubGrain::spawnInto(const Grain& parent, int depth,
                        GrainPool& pool,
                        Grain** out, int writeStart, int writeEnd,
                        juce::Random& rng) noexcept
{
    if (depth <= 0)
        return 0;

    const int numSubs = kMinSubs
        + std::min(kMaxSubs - kMinSubs, (int)(rng.nextFloat() * (float)(kMaxSubs - kMinSubs + 1)));
    int written = 0;

    for (int i = 0; i < numSubs; ++i)
    {
        const int slot = writeStart + written;
        if (slot >= writeEnd)
            break;

        Grain* sub = spawnOne(parent, numSubs, pool, rng);
        if (sub == nullptr)
            break;

        out[slot] = sub;
        ++written;

        if (depth >= 2)
        {
            const int numSubs2 = kMinSubs2
                + std::min(kMaxSubs2 - kMinSubs2,
                           (int)(rng.nextFloat() * (float)(kMaxSubs2 - kMinSubs2 + 1)));
            for (int j = 0; j < numSubs2; ++j)
            {
                const int slot2 = writeStart + written;
                if (slot2 >= writeEnd)
                    break;
                Grain* sub2 = spawnOne(*sub, numSubs2, pool, rng);
                if (sub2 == nullptr)
                    break;
                out[slot2] = sub2;
                ++written;
            }
        }
    }

    // Overlap normalisation for the family: N decorrelated grains sum in
    // power, so scale every member (parent included, via the caller) by
    // 1/sqrt(1 + N) to keep the family's loudness ≈ a single grain (R-04).
    if (written > 0)
    {
        const float famGain = 1.0f / std::sqrt(1.0f + (float)written);
        for (int k = 0; k < written; ++k)
            out[writeStart + k]->gain *= famGain;
    }
    return written;
}

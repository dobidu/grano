#include "SubGrain.h"
#include <algorithm>

static Grain* spawnOne(const Grain& parent, int subIdx, int numSubs,
                       GrainPool& pool, juce::Random& rng) noexcept
{
    juce::ignoreUnused(subIdx);

    Grain* g = pool.acquire();
    if (g == nullptr)
        return nullptr;

    const int subLen    = std::max(1, parent.lengthSamples / numSubs);
    const int maxOffset = std::max(0, parent.lengthSamples - subLen);
    const int offset    = (int)(rng.nextFloat() * (float)maxOffset);

    g->source        = parent.source;
    g->startPos      = parent.startPos + offset;
    g->lengthSamples = subLen;
    g->pitchRatio    = parent.pitchRatio * (0.9f + rng.nextFloat() * 0.2f);
    g->pan           = juce::jlimit(-1.0f, 1.0f,
                           parent.pan + (rng.nextFloat() - 0.5f) * 0.4f);
    g->shape         = parent.shape;
    g->currentPhase  = 0.0f;
    g->reversed      = parent.reversed;
    return g;
}

int SubGrain::spawnInto(const Grain& parent, int depth,
                        GrainPool& pool,
                        Grain** fifoSlots, int writeStart, int writeEnd,
                        juce::Random& rng) noexcept
{
    if (depth <= 0)
        return 0;

    const int numSubs = kMinSubs
        + (int)(rng.nextFloat() * (float)(kMaxSubs - kMinSubs + 1));
    int written = 0;

    for (int i = 0; i < numSubs; ++i)
    {
        const int slot = writeStart + written;
        if (slot >= writeEnd)
            break;

        Grain* sub = spawnOne(parent, i, numSubs, pool, rng);
        if (sub == nullptr)
            break;

        fifoSlots[slot] = sub;
        ++written;

        if (depth >= 2)
        {
            const int numSubs2 = kMinSubs2
                + (int)(rng.nextFloat() * (float)(kMaxSubs2 - kMinSubs2 + 1));
            for (int j = 0; j < numSubs2; ++j)
            {
                const int slot2 = writeStart + written;
                if (slot2 >= writeEnd)
                    break;
                Grain* sub2 = spawnOne(*sub, j, numSubs2, pool, rng);
                if (sub2 == nullptr)
                    break;
                fifoSlots[slot2] = sub2;
                ++written;
            }
        }
    }

    return written;
}

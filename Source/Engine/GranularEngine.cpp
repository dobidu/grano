#include "GranularEngine.h"
#include "EnvelopeShapes.h"
#include <cmath>

GranularEngine::GranularEngine() = default;

GranularEngine::~GranularEngine()
{
    sampleReady_.store(false, std::memory_order_relaxed);
    schedulerThread_.stopThread(2000);
}

void GranularEngine::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    reset(); // stop thread, drain FIFO, release active grains

    sampleRate_ = sampleRate;

    const int len = (int)sampleRate;
    testSample_.resize(len + 2); // +2 guard samples for interpolation
    const float twoPi = juce::MathConstants<float>::twoPi;
    for (int i = 0; i < len + 2; ++i)
        testSample_[i] = std::sin(twoPi * 440.0f * (float)i / (float)sampleRate);

    sampleReady_.store(true, std::memory_order_release);
    schedulerThread_.startThread();
}

void GranularEngine::reset()
{
    sampleReady_.store(false, std::memory_order_release);
    schedulerThread_.stopThread(1000);

    for (int i = 0; i < activeGrainCount_; ++i)
        pool_.release(activeGrains_[i]);
    activeGrainCount_ = 0;

    int s1, n1, s2, n2;
    fifo_.prepareToRead(fifo_.getNumReady(), s1, n1, s2, n2);
    for (int i = 0; i < n1; ++i) pool_.release(fifoSlots_[s1 + i]);
    for (int i = 0; i < n2; ++i) pool_.release(fifoSlots_[s2 + i]);
    fifo_.finishedRead(n1 + n2);
}

// --- SchedulerThread ---

void GranularEngine::SchedulerThread::run()
{
    while (!threadShouldExit())
    {
        engine_.scheduleGrain();
        wait(20); // ~50 grains/sec; wait() returns early on signalThreadShouldExit
    }
}

void GranularEngine::scheduleGrain() noexcept
{
    if (!sampleReady_.load(std::memory_order_acquire))
        return;

    Grain* g = pool_.acquire();
    if (g == nullptr)
        return; // pool exhausted — drop

    const int grainLen = (int)(0.1 * sampleRate_); // 100 ms
    g->source        = testSample_.data();
    g->startPos      = 0;
    g->lengthSamples = grainLen;
    g->pitchRatio    = 1.0f;
    g->pan           = 0.0f;
    g->shape         = EnvelopeShape::Hann;
    g->currentPhase  = 0.0f;

    int s1, n1, s2, n2;
    fifo_.prepareToWrite(1, s1, n1, s2, n2);
    if (n1 > 0)
    {
        fifoSlots_[s1] = g;
        fifo_.finishedWrite(1);
    }
    else if (n2 > 0)
    {
        fifoSlots_[s2] = g;
        fifo_.finishedWrite(1);
    }
    else
    {
        pool_.release(g); // FIFO full — drop grain
    }
}

// --- Audio thread ---

void GranularEngine::processBlock(juce::AudioBuffer<float>& buffer) noexcept
{
    buffer.clear();

    const int numCh = buffer.getNumChannels();
    if (numCh < 1)
        return;

    // Drain FIFO into active list
    {
        int s1, n1, s2, n2;
        fifo_.prepareToRead(fifo_.getNumReady(), s1, n1, s2, n2);
        auto addGrain = [&](Grain* g) noexcept {
            if (activeGrainCount_ < MaxActiveGrains)
                activeGrains_[activeGrainCount_++] = g;
            else
                pool_.release(g); // active list full — drop
        };
        for (int i = 0; i < n1; ++i) addGrain(fifoSlots_[s1 + i]);
        for (int i = 0; i < n2; ++i) addGrain(fifoSlots_[s2 + i]);
        fifo_.finishedRead(n1 + n2);
    }

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getWritePointer(numCh > 1 ? 1 : 0);
    const int N = buffer.getNumSamples();

    // Render grains; cull finished ones with swap-erase (no allocation)
    for (int gi = 0; gi < activeGrainCount_; )
    {
        const bool done = renderGrain(activeGrains_[gi], L, R, N);
        if (done)
        {
            pool_.release(activeGrains_[gi]);
            activeGrains_[gi] = activeGrains_[--activeGrainCount_];
        }
        else
        {
            ++gi;
        }
    }
}

bool GranularEngine::renderGrain(Grain* g, float* L, float* R, int numSamples) noexcept
{
    if (g->lengthSamples <= 0)
        return true;

    for (int n = 0; n < numSamples; ++n)
    {
        if (g->currentPhase >= 1.0f)
            return true;

        const float phase   = g->currentPhase;
        const float srcPosF = (float)g->startPos
                            + phase * (float)g->lengthSamples * g->pitchRatio;
        const int   srcInt  = (int)srcPosF;
        const float frac    = srcPosF - (float)srcInt;

        // Linear interpolation — +2 guard samples in testSample_ prevent OOB
        // for pitchRatio ≤ 2 when startPos=0 and lengthSamples ≤ sampleRate.
        const float s = g->source[srcInt] + frac * (g->source[srcInt + 1] - g->source[srcInt]);

        const float env   = applyEnvelope(g->shape, phase);
        const float lGain = 0.5f * (1.0f - g->pan);
        const float rGain = 0.5f * (1.0f + g->pan);
        L[n] += s * env * lGain;
        R[n] += s * env * rGain;

        g->currentPhase += 1.0f / (float)g->lengthSamples;
    }

    return g->currentPhase >= 1.0f;
}

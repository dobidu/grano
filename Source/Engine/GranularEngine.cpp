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
        // Determine wait interval: Pattern override or density-based.
        int waitMs;
        if (engine_.pattern_ && engine_.pattern_->isSchedulerOverrideActive())
        {
            const float overrideSec = engine_.pattern_->getNextIntervalSec();
            waitMs = (overrideSec >= 0.0f)
                ? (int)(overrideSec * 1000.0f)
                : 100;
        }
        else
        {
            const float density = engine_.pDensity_
                ? engine_.pDensity_->load(std::memory_order_relaxed) : 10.0f;
            waitMs = (int)(1000.0f / std::max(density, 0.1f));
        }
        wait(std::max(waitMs, 1));

        // Pattern fire gate: probability, Euclidean step, or audio-driven transient.
        if (engine_.pattern_ && !engine_.pattern_->shouldFireGrain())
            continue;

        engine_.scheduleGrain();
    }
}

void GranularEngine::scheduleGrain() noexcept
{
    // Prefer live SampleBuffer; fall back to 440 Hz test tone.
    const float* srcData = nullptr;
    int          srcLen  = 0;

    if (sampleSource_ != nullptr)
    {
        srcData = sampleSource_->getReadPointer();
        srcLen  = sampleSource_->getNumSamples();
    }

    if (srcData == nullptr || srcLen <= 0)
    {
        if (!sampleReady_.load(std::memory_order_acquire))
            return;
        srcData = testSample_.data();
        srcLen  = static_cast<int>(sampleRate_); // 1 second of 440 Hz sine
    }

    Grain* g = pool_.acquire();
    if (g == nullptr)
        return; // pool exhausted — drop

    const float grainSizeMs = pGrainSize_
        ? pGrainSize_->load(std::memory_order_relaxed) : 100.0f;
    const float position = pPosition_
        ? pPosition_->load(std::memory_order_relaxed) : 0.0f;
    const float posJitter = pPositionJitter_
        ? pPositionJitter_->load(std::memory_order_relaxed) : 0.0f;
    const float pitchShiftSt = pPitchShift_
        ? pPitchShift_->load(std::memory_order_relaxed) : 0.0f;
    const float pitchModSt   = pitchMod_ ? pitchMod_->getPitchModSemitones() : 0.0f;
    const float stereoSpread = pStereoSpread_
        ? pStereoSpread_->load(std::memory_order_relaxed) : 0.5f;

    const int grainLen = std::max(1, (int)(grainSizeMs * 0.001 * sampleRate_));

    // Position with jitter — clamped to valid source range.
    const float jitter = posJitter * (grainRng_.nextFloat() - 0.5f) * 2.0f;
    const float posFrac = std::clamp(position + jitter * posJitter, 0.0f, 1.0f);
    const int startPos = (int)(posFrac * (float)(srcLen - 1));

    // Pattern: quantize + spray pitch; bypass returns basePitch unchanged.
    const float totalPitchSt = pattern_
        ? pattern_->getPitchAdjustment(pitchShiftSt + pitchModSt)
        : (pitchShiftSt + pitchModSt);
    const float pitchRatio = std::pow(2.0f, totalPitchSt / 12.0f);

    // Pattern: spray duration multiplier.
    const float durMult = pattern_ ? pattern_->getDurMultiplier() : 1.0f;

    // Pattern: reverse flag.
    const bool reversed = pattern_ && pattern_->shouldReverseGrain();

    // Pan in [-1, 1]: centre at 0, spread widens L/R.
    const float pan = (stereoSpread * 2.0f - 1.0f) * (grainRng_.nextFloat() * 2.0f - 1.0f);

    const int baseLen = std::min(grainLen, srcLen - startPos);

    g->source        = srcData;
    g->startPos      = startPos;
    g->lengthSamples = std::min((int)((float)baseLen * durMult), srcLen - startPos);
    g->pitchRatio    = pitchRatio;
    g->pan           = std::clamp(pan, -1.0f, 1.0f);
    g->shape         = EnvelopeShape::Hann;
    g->currentPhase  = 0.0f;
    g->reversed      = reversed;

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

    // If a new sample buffer was swapped in, all active grains carry stale source
    // pointers. Drain them before the new buffer's grains arrive.
    if (sampleSource_ != nullptr && sampleSource_->trySwap())
    {
        for (int i = 0; i < activeGrainCount_; ++i)
            pool_.release(activeGrains_[i]);
        activeGrainCount_ = 0;

        int s1, n1, s2, n2;
        fifo_.prepareToRead(fifo_.getNumReady(), s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) pool_.release(fifoSlots_[s1 + i]);
        for (int i = 0; i < n2; ++i) pool_.release(fifoSlots_[s2 + i]);
        fifo_.finishedRead(n1 + n2);
    }

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

    // Apply master volume.
    const float volDb  = pMasterVolume_
        ? pMasterVolume_->load(std::memory_order_relaxed) : 0.0f;
    const float volGain = juce::Decibels::decibelsToGain(volDb, -60.0f);
    for (int n = 0; n < N; ++n) { L[n] *= volGain; R[n] *= volGain; }

    // Write snapshot for UI particle rendering (read by getGrainSnapshots on UI thread).
    const int snapLen   = sampleSource_ != nullptr ? sampleSource_->getNumSamples() : 0;
    const int snapCount = std::min(activeGrainCount_, MaxActiveGrains);
    for (int i = 0; i < snapCount; ++i)
    {
        const auto* g = activeGrains_[i];
        const float srcFrac = snapLen > 0
            ? std::clamp((float)g->startPos / (float)snapLen, 0.0f, 1.0f)
            : 0.0f;
        grainSnapshots_[static_cast<std::size_t>(i)] = { srcFrac, applyEnvelope(g->shape, g->currentPhase) };
    }
    grainSnapshotCount_.store(snapCount, std::memory_order_release);
}

int GranularEngine::getGrainSnapshots(GrainSnapshot* out, int maxCount) const noexcept
{
    const int count = std::min(grainSnapshotCount_.load(std::memory_order_acquire), maxCount);
    for (int i = 0; i < count; ++i)
        out[i] = grainSnapshots_[static_cast<std::size_t>(i)];
    return count;
}

bool GranularEngine::renderGrain(Grain* g, float* L, float* R, int numSamples) noexcept
{
    if (g->lengthSamples <= 0)
        return true;

    for (int n = 0; n < numSamples; ++n)
    {
        if (g->currentPhase >= 1.0f)
            return true;

        const float phase     = g->currentPhase;
        // Reversed: read from end of grain region back to start.
        const float readPhase = g->reversed ? (1.0f - phase) : phase;
        const float srcPosF   = (float)g->startPos
                              + readPhase * (float)g->lengthSamples * g->pitchRatio;
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

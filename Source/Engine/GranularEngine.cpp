#include "GranularEngine.h"
#include "EnvelopeShapes.h"
#include "../Modulation/ModMatrix.h"
#include <cmath>

GranularEngine::GranularEngine() = default;

GranularEngine::~GranularEngine() = default;

void GranularEngine::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    reset(); // release active grains, reset sample clock

    sampleRate_ = sampleRate;

    const int len = (int)sampleRate;
    testSample_.resize((size_t)len + 2); // +2 guard samples for interpolation
    const float twoPi = juce::MathConstants<float>::twoPi;
    for (int i = 0; i < len + 2; ++i)
        testSample_[(size_t)i] = std::sin(twoPi * 440.0f * (float)i / (float)sampleRate);

    sampleReady_ = true;
}

void GranularEngine::reset()
{
    for (int i = 0; i < activeGrainCount_; ++i)
        pool_.release(activeGrains_[(size_t)i]);
    activeGrainCount_ = 0;
    grainSnapshotCount_.store(0, std::memory_order_release);

    samplesToNext_  = 0.0;
    lastAnchoredStep_ = kNoStep;
    loopTimeSec_    = 0.0;
    prevLoopOn_     = false;
    lastMasterGain_ = -1.0f;
    loopSweepFrac_.store(0.0f, std::memory_order_relaxed);
}

// --- Sample-clock scheduler (audio thread) ---

double GranularEngine::nextIntervalSamples(int mode) noexcept
{
    double sec = -1.0;
    if (mode != 0 && pattern_ != nullptr)
        sec = (double)pattern_->getNextIntervalSec();

    if (sec <= 0.0) // Free mode: density + stochastic distribution
    {
        float density = pDensity_ ? pDensity_->load(std::memory_order_relaxed) : 10.0f;
        if (modMatrix_)
            density = juce::jlimit(1.0f, 100.0f,
                density + modMatrix_->getModOffset(ModMatrix::kDensity) * 99.0f);
        const float meanMs = 1000.0f / std::max(density, 0.1f);
        const int distIdx = pStochasticDist_
            ? (int)pStochasticDist_->load(std::memory_order_relaxed) : 0;
        const auto dist = static_cast<StochasticTiming::Distribution>(
            juce::jlimit(0, 5, distIdx));
        sec = (double)StochasticTiming::nextIntervalMs(meanMs, dist, grainRng_) * 0.001;
    }
    return std::max(1.0, sec * sampleRate_);
}

void GranularEngine::fireEvent(long long step, int offset) noexcept
{
    if (pattern_ != nullptr)
    {
        const bool fire = step >= 0 ? pattern_->shouldFireGrainAtStep(step)
                                    : pattern_->shouldFireGrain();
        if (!fire)
            return;
    }
    scheduleGrain(offset);
}

void GranularEngine::runScheduler(int numSamples) noexcept
{
    const int mode = pattern_ ? pattern_->getActiveTriggerMode() : 0;

    const bool anchored = (mode == 1 || mode == 2) && transportPlaying_ && ppqValid_
                          && bpm_ > 0.0 && pattern_ != nullptr;
    if (anchored)
    {
        // Host-grid steps: fire on every step boundary in [ppq_, ppq_ + block).
        const double spb       = sampleRate_ * 60.0 / bpm_; // samples per beat
        const double stepBeats = std::max(1.0e-4, pattern_->getStepBeats());
        auto k = (long long)std::ceil(ppq_ / stepBeats - 1.0e-9);
        // A boundary that lands on a block edge can be seen by both blocks
        // through float rounding — never fire a step twice while the
        // transport runs forward continuously.
        if (lastAnchoredStep_ != kNoStep && k <= lastAnchoredStep_ && lastAnchoredStep_ - k < 2)
            k = lastAnchoredStep_ + 1;
        for (;; ++k)
        {
            const double off = ((double)k * stepBeats - ppq_) * spb;
            if (off >= (double)numSamples - 1.0e-6) // edge belongs to the next block
            {
                // Hand over to the free-running clock seamlessly if the
                // transport stops before the next step.
                samplesToNext_ = off - (double)numSamples;
                break;
            }
            fireEvent(k, std::max(0, (int)off));
            lastAnchoredStep_ = k;
        }
        return;
    }
    lastAnchoredStep_ = kNoStep;

    while (samplesToNext_ < (double)numSamples)
    {
        fireEvent(-1, std::max(0, (int)samplesToNext_));
        samplesToNext_ += nextIntervalSamples(mode);
    }
    samplesToNext_ -= (double)numSamples;
}

void GranularEngine::addActive(Grain* g) noexcept
{
    if (activeGrainCount_ < MaxActiveGrains)
        activeGrains_[(size_t)activeGrainCount_++] = g;
    else
        pool_.release(g); // active list full — drop
}

void GranularEngine::scheduleGrain(int offset) noexcept
{
    // Source priority: Spectral > Feedback > SampleBuffer > 440 Hz sine.
    const float* srcData = nullptr;
    int          srcLen  = 0;
    int          wrapLen = 0;

    const bool spectralOn = spectralSource_ != nullptr &&
        pSpectralEnabled_ != nullptr &&
        pSpectralEnabled_->load(std::memory_order_relaxed) > 0.5f;

    const bool feedbackOn = !spectralOn &&
        feedbackSource_ != nullptr &&
        pFeedbackEnabled_ != nullptr &&
        pFeedbackEnabled_->load(std::memory_order_relaxed) > 0.5f;

    if (spectralOn)
    {
        srcData = spectralSource_->getReadPointer();
        srcLen  = spectralSource_->getNumSamples();
    }
    else if (feedbackOn)
    {
        srcData = feedbackSource_->getReadPointer();
        srcLen  = feedbackSource_->getNumSamples();
        wrapLen = srcLen;
    }
    else if (bankSource_ != nullptr)
    {
        float weights[MultiSampleBank::kNumSlots];
        for (int s = 0; s < MultiSampleBank::kNumSlots; ++s)
            weights[s] = pSlotWeight_[s]
                ? pSlotWeight_[s]->load(std::memory_order_relaxed)
                : 1.0f;
        auto [ptr, len] = bankSource_->pickSlot(grainRng_, weights);
        srcData = ptr;
        srcLen  = len;
    }

    if (srcData == nullptr || srcLen <= 1)
    {
        if (!sampleReady_)
            return;
        srcData = testSample_.data();
        srcLen  = static_cast<int>(sampleRate_); // 1 second of 440 Hz sine
    }

    Grain* g = pool_.acquire();
    if (g == nullptr)
        return; // pool exhausted — drop

    // Mod-matrix offsets scale by the destination's full parameter range,
    // then the result is clamped back into that range.
    float grainSizeMs = pGrainSize_
        ? pGrainSize_->load(std::memory_order_relaxed) : 100.0f;
    if (modMatrix_)
        grainSizeMs = std::clamp(
            grainSizeMs + modMatrix_->getModOffset(ModMatrix::kGrainSize) * 495.0f,
            5.0f, 500.0f);
    float position = pPosition_
        ? pPosition_->load(std::memory_order_relaxed) : 0.0f;
    if (modMatrix_)
        position += modMatrix_->getModOffset(ModMatrix::kPosition);
    const float posJitter = juce::jlimit(0.0f, 1.0f,
        (pPositionJitter_ ? pPositionJitter_->load(std::memory_order_relaxed) : 0.0f)
        + (modMatrix_ ? modMatrix_->getModOffset(ModMatrix::kPositionJitter) : 0.0f));
    const float pitchShiftSt = pPitchShift_
        ? pPitchShift_->load(std::memory_order_relaxed) : 0.0f;
    const float matrixPitchSt = modMatrix_
        ? modMatrix_->getModOffset(ModMatrix::kPitchShift) * 24.0f : 0.0f;
    const float motionPitchSt = pitchMod_ ? pitchMod_->getPitchModSemitones() : 0.0f;
    float stereoSpread = pStereoSpread_
        ? pStereoSpread_->load(std::memory_order_relaxed) : 0.5f;
    if (modMatrix_)
        stereoSpread = std::clamp(
            stereoSpread + modMatrix_->getModOffset(ModMatrix::kStereoSpread), 0.0f, 1.0f);

    const int grainLen = std::max(1, (int)(grainSizeMs * 0.001 * sampleRate_));

    // Loop sweep: the base position advances with rendered samples (including
    // this grain's offset inside the block), wrapping once per source duration.
    const bool loopOn = pLoop_ != nullptr &&
        pLoop_->load(std::memory_order_relaxed) > 0.5f;
    if (loopOn && wrapLen == 0)
    {
        const double srcDurSec = (double)srcLen / sampleRate_;
        const double t = loopTimeSec_ + (double)offset / sampleRate_;
        position = (float)((double)position + t / std::max(srcDurSec, 0.001));
        position -= std::floor(position); // wrap into [0, 1)
    }

    // Position with jitter — jitter is linear in the knob value (a setting of
    // J scatters +/- J), clamped to valid source range.
    const float jitter  = posJitter * (grainRng_.nextFloat() - 0.5f) * 2.0f;
    const float posFrac = std::clamp(position + jitter, 0.0f, 1.0f);

    // Pattern: quantize + spray applied to the played pitch (knob + matrix).
    // Motion's wow/flutter/drift is added AFTER quantize so a scale doesn't
    // snap the tape wobble away (R-19).
    const float quantizedSt = pattern_
        ? pattern_->getPitchAdjustment(pitchShiftSt + matrixPitchSt)
        : (pitchShiftSt + matrixPitchSt);
    const float pitchRatio = std::pow(2.0f, (quantizedSt + motionPitchSt) / 12.0f);

    const float durMult  = pattern_ ? pattern_->getDurMultiplier() : 1.0f;
    const bool  reversed = pattern_ && pattern_->shouldReverseGrain();

    // Pan in [-1, 1]: spread 0 = centre (mono), 1 = full random width (R-02).
    const float pan = stereoSpread * (grainRng_.nextFloat() * 2.0f - 1.0f);

    *g = Grain{};
    g->source        = srcData;
    g->sourceLen     = srcLen;
    g->wrapLen       = wrapLen;
    g->lengthSamples = std::max(1, (int)((float)grainLen * durMult));
    g->pitchRatio    = pitchRatio;
    g->pan           = std::clamp(pan, -1.0f, 1.0f);
    const int shapeIdx = pEnvelopeShape_
        ? (int)std::clamp(pEnvelopeShape_->load(std::memory_order_relaxed), 0.0f, 4.0f) : 0;
    g->shape      = static_cast<EnvelopeShape>(shapeIdx);
    g->elapsed    = 0;
    g->reversed   = reversed;
    g->startDelay = offset;

    if (wrapLen > 0)
    {
        // Feedback ring: read only audio already written, behind the write
        // head, and never where the writer will be during this grain (R-12).
        // Position 0 = most recent audio, 1 = oldest.
        const double p = (double)pitchRatio;
        const int maxLen = std::max(1, (int)((double)(wrapLen - 8) / (p + 1.0)));
        g->lengthSamples = std::min(g->lengthSamples, maxLen);
        const int span   = (int)std::ceil((double)g->lengthSamples * p) + 2;
        // The ring is written after the engine renders each block, so
        // everything before writePos() is valid audio. The writer advances
        // offset + lengthSamples (+ one block of slack) during this grain's
        // life — keep the read window clear of it.
        const int W      = feedbackSource_->writePos();
        const int newest = W - 1 - span;
        const int oldest = W - wrapLen + offset + g->lengthSamples + 4096;
        const int start  = newest - (int)((float)std::max(0, newest - oldest) * posFrac);
        g->startPos = ((start % wrapLen) + wrapLen) % wrapLen;
    }
    else
    {
        g->startPos = (int)(posFrac * (float)(srcLen - 1));
        fitGrainToSource(*g); // whole read span inside the source (R-03)
    }

    // Overlap normalisation (R-04): expected concurrent grains =
    // event rate × grain duration. Decorrelated grains sum in power.
    {
        double eventsPerSec = 0.0;
        const int mode = pattern_ ? pattern_->getActiveTriggerMode() : 0;
        const float intervalSec = (mode != 0 && pattern_) ? pattern_->getNextIntervalSec() : -1.0f;
        if (intervalSec > 0.0f && mode != 3)
            eventsPerSec = 1.0 / (double)intervalSec;
        else if (mode == 0)
        {
            float density = pDensity_ ? pDensity_->load(std::memory_order_relaxed) : 10.0f;
            if (modMatrix_)
                density = juce::jlimit(1.0f, 100.0f,
                    density + modMatrix_->getModOffset(ModMatrix::kDensity) * 99.0f);
            eventsPerSec = density;
        }
        const double overlap = eventsPerSec * (double)g->lengthSamples / sampleRate_;
        g->gain = (float)(1.0 / std::sqrt(std::max(1.0, overlap)));
    }

    // Sub-grain recursion (depth 1 or 2).
    const int depth = pSubGrainDepth_
        ? (int)pSubGrainDepth_->load(std::memory_order_relaxed) : 0;
    Grain* family[SubGrain::kMaxFamily];
    int nSubs = 0;
    if (depth > 0)
        nSubs = SubGrain::spawnInto(*g, depth, pool_, family, 0, SubGrain::kMaxFamily, grainRng_);
    if (nSubs > 0)
        g->gain *= 1.0f / std::sqrt(1.0f + (float)nSubs);

    spawnCount_.fetch_add(1, std::memory_order_relaxed);
    addActive(g);
    for (int i = 0; i < nSubs; ++i)
        addActive(family[i]);
}

// --- Audio thread ---

void GranularEngine::processBlock(juce::AudioBuffer<float>& buffer) noexcept
{
    buffer.clear();

    const int numCh = buffer.getNumChannels();
    if (numCh < 1)
        return;
    const int N = buffer.getNumSamples();

    // If a new sample buffer was swapped in, all active grains carry stale
    // source pointers — drop them before scheduling from the new buffer.
    if (bankSource_ != nullptr && bankSource_->trySwapAll())
    {
        for (int i = 0; i < activeGrainCount_; ++i)
            pool_.release(activeGrains_[(size_t)i]);
        activeGrainCount_ = 0;
    }

    // Loop sweep clock: rising edge restarts the sweep.
    const bool loopOn = pLoop_ != nullptr &&
        pLoop_->load(std::memory_order_relaxed) > 0.5f;
    if (loopOn && !prevLoopOn_)
        loopTimeSec_ = 0.0;
    prevLoopOn_ = loopOn;

    runScheduler(N);

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getWritePointer(numCh > 1 ? 1 : 0);

    // Render grains; cull finished ones with swap-erase (no allocation)
    for (int gi = 0; gi < activeGrainCount_; )
    {
        const bool done = renderGrain(activeGrains_[(size_t)gi], L, R, N);
        if (done)
        {
            pool_.release(activeGrains_[(size_t)gi]);
            activeGrains_[(size_t)gi] = activeGrains_[(size_t)--activeGrainCount_];
        }
        else
        {
            ++gi;
        }
    }

    // Advance the loop clock by the rendered block; publish sweep for the UI.
    if (loopOn)
    {
        loopTimeSec_ += (double)N / sampleRate_;
        const int len0 = bankSource_ != nullptr ? bankSource_->getNumSamples(0) : 0;
        const double dur = len0 > 0 ? (double)len0 / sampleRate_ : 1.0;
        const double frac = loopTimeSec_ / std::max(dur, 0.001);
        loopSweepFrac_.store((float)(frac - std::floor(frac)), std::memory_order_relaxed);
    }
    else
    {
        loopSweepFrac_.store(0.0f, std::memory_order_relaxed);
    }

    // Write snapshot for UI particle rendering (read by getGrainSnapshots on UI thread).
    const int snapLen   = bankSource_ != nullptr ? bankSource_->getNumSamples(0) : 0;
    int snapCount = 0;
    for (int i = 0; i < activeGrainCount_ && snapCount < MaxActiveGrains; ++i)
    {
        const auto* g = activeGrains_[(size_t)i];
        if (g->startDelay > 0 || g->wrapLen > 0)
            continue; // not sounding yet / feedback ring (no waveform position)
        const float srcFrac = snapLen > 0
            ? std::clamp((float)g->startPos / (float)snapLen, 0.0f, 1.0f)
            : 0.0f;
        // Vertical scatter derived from the grain itself (Knuth hash of its
        // start position), not the array index — stable per grain and uniform
        // over [0,1) on every stdlib (std::hash<int> is identity on GCC/Clang).
        const float yFrac = (float)(((unsigned)g->startPos * 2654435761u) & 0xFFFFu)
                            / 65536.0f;
        grainSnapshots_[(size_t)snapCount++] =
            { srcFrac, applyEnvelope(g->shape, g->phase()), yFrac };
    }
    grainSnapshotCount_.store(snapCount, std::memory_order_release);
}

void GranularEngine::applyMasterGain(juce::AudioBuffer<float>& buffer) noexcept
{
    float volDb = pMasterVolume_
        ? pMasterVolume_->load(std::memory_order_relaxed) : 0.0f;
    if (modMatrix_)
        volDb = std::clamp(
            volDb + modMatrix_->getModOffset(ModMatrix::kMasterVol) * 66.0f,
            -60.0f, 6.0f);
    const float target = juce::Decibels::decibelsToGain(volDb, -60.0f);
    const float start  = lastMasterGain_ < 0.0f ? target : lastMasterGain_;
    lastMasterGain_ = target;

    const int numCh = buffer.getNumChannels();
    const int N     = buffer.getNumSamples();
    if (numCh < 1 || N <= 0)
        return;
    // Ramp per channel; skip a channel that aliases channel 0 (mono layouts).
    const float* ch0 = buffer.getReadPointer(0);
    for (int c = 0; c < numCh; ++c)
    {
        if (c > 0 && buffer.getReadPointer(c) == ch0)
            continue;
        if (start == target)
            buffer.applyGain(c, 0, N, target);
        else
            buffer.applyGainRamp(c, 0, N, start, target);
    }
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
    if (g->lengthSamples <= 0 || g->source == nullptr)
        return true;

    int n = 0;
    if (g->startDelay > 0)
    {
        if (g->startDelay >= numSamples)
        {
            g->startDelay -= numSamples;
            return false; // not sounding in this block yet
        }
        n = g->startDelay;
        g->startDelay = 0;
    }

    const double p     = (double)g->pitchRatio;
    const double base  = (double)g->startPos;
    const int    len   = g->lengthSamples;
    const float  invLen = 1.0f / (float)len;
    const float  lGain = 0.5f * (1.0f - g->pan) * g->gain;
    const float  rGain = 0.5f * (1.0f + g->pan) * g->gain;
    const int    wrap  = g->wrapLen;
    const bool   aliased = (R == L);

    for (; n < numSamples; ++n)
    {
        const int e = g->elapsed;
        if (e >= len)
            return true;

        // Integer elapsed counter + double read position: exact pitch, full
        // sub-sample precision on long sources (R-05).
        const double readIdx = g->reversed ? (double)(len - e) : (double)e;
        const double srcPos  = base + readIdx * p;
        int          srcInt  = (int)srcPos;
        const float  frac    = (float)(srcPos - (double)srcInt);

        float s;
        if (wrap > 0)
        {
            srcInt %= wrap;
            const int nxt = srcInt + 1 < wrap ? srcInt + 1 : 0;
            s = g->source[srcInt] + frac * (g->source[nxt] - g->source[srcInt]);
        }
        else
        {
            // fitGrainToSource keeps reads in [0, sourceLen]; guards cover +1.
            if (srcInt < 0 || srcInt > g->sourceLen)
                return true;
            s = g->source[srcInt] + frac * (g->source[srcInt + 1] - g->source[srcInt]);
        }

        const float env = applyEnvelope(g->shape, (float)e * invLen);
        if (aliased)
            L[n] += s * env * (lGain + rGain); // mono alias: both pan gains land on one channel
        else
        {
            L[n] += s * env * lGain;
            R[n] += s * env * rGain;
        }
        g->elapsed = e + 1;
    }

    return g->elapsed >= len;
}

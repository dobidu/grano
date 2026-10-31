#include "Pattern.h"
#include <cmath>
#include <algorithm>

// ── Scale tables ──────────────────────────────────────────────────────────────
// Semitone intervals from root (mod-12), sorted ascending. Unused slots = 0.
const Pattern::Scale Pattern::kScales[Pattern::kNumScales] = {
    // Chromatic
    {{ 0,1,2,3,4,5,6,7,8,9,10,11 }, 12},
    // Major
    {{ 0,2,4,5,7,9,11, 0,0,0,0,0 }, 7},
    // Natural Minor
    {{ 0,2,3,5,7,8,10, 0,0,0,0,0 }, 7},
    // Dorian
    {{ 0,2,3,5,7,9,10, 0,0,0,0,0 }, 7},
    // Phrygian
    {{ 0,1,3,5,7,8,10, 0,0,0,0,0 }, 7},
    // Lydian
    {{ 0,2,4,6,7,9,11, 0,0,0,0,0 }, 7},
    // Mixolydian
    {{ 0,2,4,5,7,9,10, 0,0,0,0,0 }, 7},
    // Pentatonic Major
    {{ 0,2,4,7,9, 0,0,0,0,0,0,0  }, 5},
    // Pentatonic Minor
    {{ 0,3,5,7,10,0,0,0,0,0,0,0  }, 5},
    // Whole Tone
    {{ 0,2,4,6,8,10,   0,0,0,0,0,0 }, 6},
    // Octatonic (half-whole diminished)
    {{ 0,1,3,4,6,7,9,10,0,0,0,0  }, 8},
};

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void Pattern::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
    const auto coef = [sampleRate](double tauSec)
    { return (float)(1.0 - std::exp(-1.0 / (tauSec * sampleRate))); };
    fastAttack_  = coef(0.001);
    fastRelease_ = coef(0.030);
    slowCoef_    = coef(0.400);
    reset();
}

void Pattern::reset() noexcept
{
    euclidStep_        = 0;
    fastEnv_           = 0.0f;
    slowEnv_           = 0.0f;
    transientArmed_    = true;
    transientCooldown_ = 0;
    transientFired_.store(false, std::memory_order_relaxed);
    syncIntervalAtomic_.store(0.25f, std::memory_order_relaxed);
}

void Pattern::setParamPointers(
    std::atomic<float>* enabled,
    std::atomic<float>* triggerMode,
    std::atomic<float>* syncDivision,
    std::atomic<float>* euclidPulses,
    std::atomic<float>* euclidSteps,
    std::atomic<float>* euclidRotation,
    std::atomic<float>* transientSens,
    std::atomic<float>* probability,
    std::atomic<float>* reverseProb,
    std::atomic<float>* quantizeScale,
    std::atomic<float>* spray) noexcept
{
    pEnabled_         = enabled;
    pTriggerMode_     = triggerMode;
    pSyncDivision_    = syncDivision;
    pEuclidPulses_    = euclidPulses;
    pEuclidSteps_     = euclidSteps;
    pEuclidRotation_  = euclidRotation;
    pTransientSens_   = transientSens;
    pProbability_     = probability;
    pReverseProb_     = reverseProb;
    pQuantizeScale_   = quantizeScale;
    pSpray_           = spray;
}

// ── Scale quantization ────────────────────────────────────────────────────────

float Pattern::quantizeToScale(float pitchSt, int scaleIdx) noexcept
{
    if (scaleIdx < 0 || scaleIdx >= kNumScales)
        return pitchSt;
    const Scale& sc = kScales[scaleIdx];
    if (sc.count <= 0) return pitchSt;

    // Fold to octave offset + mod-12 remainder
    const float octave = std::floor(pitchSt / 12.0f);
    const float mod12  = pitchSt - octave * 12.0f;

    // Find nearest scale note; also check first note + 12 for wrap-around
    float bestDist = 999.0f;
    float bestNote = 0.0f;
    for (int i = 0; i < sc.count; ++i)
    {
        const float note = (float)sc.notes[i];
        const float dist = std::abs(mod12 - note);
        if (dist < bestDist) { bestDist = dist; bestNote = note; }
    }
    {
        const float note = (float)sc.notes[0] + 12.0f;
        const float dist = std::abs(mod12 - note);
        if (dist < bestDist) { bestNote = note; }
    }
    return octave * 12.0f + bestNote;
}

// ── Euclidean rhythm ─────────────────────────────────────────────────────────

Pattern::EuclidPattern Pattern::buildEuclidean(int pulses, int steps, int rotation) noexcept
{
    EuclidPattern pat{};
    if (steps <= 0 || pulses <= 0) return pat;
    pulses = std::min(pulses, steps);

    // Bresenham-style Euclidean rhythm formula (Toussaint 2005):
    // pat[i] is a hit when floor((i+rot)*k/n) != floor((i+rot-1)*k/n)
    // Equivalent, simpler form: pat[i] = ((i+rot)*pulses % steps) < pulses
    for (int i = 0; i < steps; ++i)
        pat[i] = (((i + rotation) * pulses) % steps) < pulses;

    return pat;
}

void Pattern::rebuildEuclidIfNeeded() noexcept
{
    const int p = pEuclidPulses_   ? (int)pEuclidPulses_->load(std::memory_order_relaxed)   : 4;
    const int s = pEuclidSteps_    ? (int)pEuclidSteps_->load(std::memory_order_relaxed)    : 8;
    const int r = pEuclidRotation_ ? (int)pEuclidRotation_->load(std::memory_order_relaxed) : 0;
    if (p == cachedPulses_ && s == cachedSteps_ && r == cachedRotation_)
        return;
    euclidPattern_    = buildEuclidean(p, s, r);
    euclidPatternLen_ = std::max(s, 1);
    euclidStep_       = 0;
    cachedPulses_     = p;
    cachedSteps_      = s;
    cachedRotation_   = r;
}

// ── Audio thread: processBlock ────────────────────────────────────────────────

void Pattern::processBlock(const juce::AudioBuffer<float>& inputBuffer, double bpm) noexcept
{
    if (pEnabled_ == nullptr || pEnabled_->load(std::memory_order_relaxed) < 0.5f)
        return;

    const int mode = pTriggerMode_
        ? (int)pTriggerMode_->load(std::memory_order_relaxed) : 0;

    // ── Sync interval (Sync mode; Euclidean uses it as its step clock) ───────
    if ((mode == 1 || mode == 2) && bpm > 0.0)
    {
        const int divIdx = pSyncDivision_
            ? (int)pSyncDivision_->load(std::memory_order_relaxed) : 1;

        juce::ignoreUnused(divIdx);
        const float beatSec = (float)(60.0 / bpm);
        syncIntervalAtomic_.store(beatSec * (float)getStepBeats(), std::memory_order_relaxed);
    }

    // ── Audio-driven transient detection ─────────────────────────────────────
    // Per-sample fast/slow energy envelopes on L+R with time constants in
    // seconds, so sensitivity is independent of the host block size and a
    // right-only input is heard (R-29). Onset = fast envelope rising above
    // the slow (background) envelope by the sensitivity-dependent ratio.
    if (mode == 3 && inputBuffer.getNumChannels() > 0)
    {
        const float sens = pTransientSens_
            ? pTransientSens_->load(std::memory_order_relaxed) : 0.5f;
        const float ratioThresh = 1.5f + (1.0f - sens) * 6.5f; // 8x (low) .. 1.5x (high)

        const float* chL = inputBuffer.getReadPointer(0);
        const float* chR = inputBuffer.getNumChannels() > 1 ? inputBuffer.getReadPointer(1) : chL;
        const int    N   = inputBuffer.getNumSamples();
        const int    cooldownSamples = (int)(kTransientCooldownMs * 0.001f * (float)sampleRate_);

        for (int n = 0; n < N; ++n)
        {
            const float e = 0.5f * (chL[n] * chL[n] + chR[n] * chR[n]);
            fastEnv_ += (e > fastEnv_ ? fastAttack_ : fastRelease_) * (e - fastEnv_);
            slowEnv_ += slowCoef_ * (e - slowEnv_);

            // Hysteresis: after an onset, re-arm only once the fast envelope
            // has fallen back near the background — a decaying tail must not
            // re-trigger when the cooldown ends.
            if (!transientArmed_ && fastEnv_ < 0.5f * ratioThresh * (slowEnv_ + 1.0e-7f))
                transientArmed_ = true;

            if (transientCooldown_ > 0)
            {
                --transientCooldown_;
                continue;
            }
            if (transientArmed_ && fastEnv_ > 1.0e-6f
                && fastEnv_ > ratioThresh * (slowEnv_ + 1.0e-7f))
            {
                transientFired_.store(true, std::memory_order_relaxed);
                transientCooldown_ = cooldownSamples;
                transientArmed_    = false;
            }
        }
    }
}

// ── Scheduler API (audio thread — sample clock) ──────────────────────────────

int Pattern::getActiveTriggerMode() const noexcept
{
    if (pEnabled_ == nullptr || pEnabled_->load(std::memory_order_relaxed) < 0.5f)
        return 0;
    return juce::jlimit(0, 3, pTriggerMode_
        ? (int)pTriggerMode_->load(std::memory_order_relaxed) : 0);
}

double Pattern::getStepBeats() const noexcept
{
    // Division lengths in quarter-note beats.
    // Order: 1/4, 1/8, 1/16, 1/32, 1/64, 1/4T, 1/8T, 1/16T, 1/4D, 1/8D
    static constexpr double kDivBeats[] = {
        1.0, 0.5, 0.25, 0.125, 0.0625,
        2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0,
        1.5, 0.75
    };
    const int idx = juce::jlimit(0, 9, pSyncDivision_
        ? (int)pSyncDivision_->load(std::memory_order_relaxed) : 1);
    return kDivBeats[idx];
}

bool Pattern::shouldFireGrainAtStep(long long step) noexcept
{
    if (getActiveTriggerMode() == 2)
    {
        rebuildEuclidIfNeeded();
        const long long len = std::max(euclidPatternLen_, 1);
        const long long idx = ((step % len) + len) % len;
        if (!euclidPattern_[(size_t)idx])
            return false;
    }
    const float prob = pProbability_
        ? pProbability_->load(std::memory_order_relaxed) : 1.0f;
    return rng_.nextFloat() < prob;
}

// ── Free-running scheduler API ────────────────────────────────────────────────

bool Pattern::isSchedulerOverrideActive() const noexcept
{
    if (pEnabled_ == nullptr || pEnabled_->load(std::memory_order_relaxed) < 0.5f)
        return false;
    const int mode = pTriggerMode_
        ? (int)pTriggerMode_->load(std::memory_order_relaxed) : 0;
    return mode != 0;
}

float Pattern::getNextIntervalSec() noexcept
{
    if (!isSchedulerOverrideActive()) return -1.0f;

    const int mode = pTriggerMode_
        ? (int)pTriggerMode_->load(std::memory_order_relaxed) : 0;

    switch (mode)
    {
        case 1: // Sync
            return syncIntervalAtomic_.load(std::memory_order_relaxed);

        case 2: // Euclidean — one pattern step per sync division, tempo-locked
        {
            // syncIntervalAtomic_ is updated from host BPM + syncDivision in
            // processBlock (shared with Sync mode). Fallback to 0.125 s/step
            // (8 steps/s) when the host provides no usable tempo.
            const float stepSec = syncIntervalAtomic_.load(std::memory_order_relaxed);
            return stepSec > 0.0f ? stepSec : 0.125f;
        }

        case 3: // Audio-driven — poll at fixed short interval
            return kPollingIntervalSec;

        default:
            return -1.0f;
    }
}

bool Pattern::shouldFireGrain() noexcept
{
    if (pEnabled_ == nullptr || pEnabled_->load(std::memory_order_relaxed) < 0.5f)
        return true;

    const int mode = pTriggerMode_
        ? (int)pTriggerMode_->load(std::memory_order_relaxed) : 0;

    // Euclidean gate: advance step counter, check hit
    if (mode == 2)
    {
        rebuildEuclidIfNeeded();
        const bool hit = euclidPattern_[euclidStep_ % std::max(euclidPatternLen_, 1)];
        euclidStep_ = (euclidStep_ + 1) % std::max(euclidPatternLen_, 1);
        if (!hit) return false;
    }

    // Audio-driven gate: only fire when transient detected
    if (mode == 3)
    {
        const bool fired = transientFired_.exchange(false, std::memory_order_relaxed);
        if (!fired) return false;
    }

    // Probability gate
    const float prob = pProbability_
        ? pProbability_->load(std::memory_order_relaxed) : 1.0f;
    return rng_.nextFloat() < prob;
}

bool Pattern::shouldReverseGrain() noexcept
{
    if (pEnabled_ == nullptr || pEnabled_->load(std::memory_order_relaxed) < 0.5f)
        return false;
    const float rp = pReverseProb_
        ? pReverseProb_->load(std::memory_order_relaxed) : 0.0f;
    return rng_.nextFloat() < rp;
}

float Pattern::getPitchAdjustment(float basePitchSt) noexcept
{
    if (pEnabled_ == nullptr || pEnabled_->load(std::memory_order_relaxed) < 0.5f)
        return basePitchSt;

    float pitch = basePitchSt;

    // Quantize to scale (index 0 = Chromatic = no-op since all notes are valid)
    const int scaleIdx = pQuantizeScale_
        ? (int)pQuantizeScale_->load(std::memory_order_relaxed) : 0;
    if (scaleIdx > 0)
        pitch = quantizeToScale(pitch, scaleIdx);

    // Spray pitch randomness
    const float sprayAmt = pSpray_
        ? pSpray_->load(std::memory_order_relaxed) : 0.0f;
    if (sprayAmt > 0.001f)
        pitch += (rng_.nextFloat() * 2.0f - 1.0f) * sprayAmt * kMaxSprayPitchSt;

    return pitch;
}

float Pattern::getDurMultiplier() noexcept
{
    if (pEnabled_ == nullptr || pEnabled_->load(std::memory_order_relaxed) < 0.5f)
        return 1.0f;
    const float sprayAmt = pSpray_
        ? pSpray_->load(std::memory_order_relaxed) : 0.0f;
    if (sprayAmt < 0.001f) return 1.0f;
    // Symmetric in log-time: spray 1 → ×0.5 .. ×2 (was ×1 .. ×3, only longer).
    return std::exp2((rng_.nextFloat() * 2.0f - 1.0f) * sprayAmt);
}

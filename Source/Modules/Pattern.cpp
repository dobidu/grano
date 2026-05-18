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
    reset();
}

void Pattern::reset() noexcept
{
    euclidStep_        = 0;
    energyFollower_    = 0.0f;
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

    // ── Sync interval ────────────────────────────────────────────────────────
    if (mode == 1 && bpm > 0.0)
    {
        const int divIdx = pSyncDivision_
            ? (int)pSyncDivision_->load(std::memory_order_relaxed) : 1;

        // Division multipliers relative to one quarter note (60/bpm seconds).
        // Order: 1/4, 1/8, 1/16, 1/32, 1/64, 1/4T, 1/8T, 1/16T, 1/4D, 1/8D
        static constexpr float kDivMult[] = {
            1.0f, 0.5f, 0.25f, 0.125f, 0.0625f,
            2.0f/3.0f, 1.0f/3.0f, 1.0f/6.0f,
            1.5f, 0.75f
        };
        const int   idx     = juce::jlimit(0, 9, divIdx);
        const float beatSec = (float)(60.0 / bpm);
        syncIntervalAtomic_.store(beatSec * kDivMult[idx], std::memory_order_relaxed);
    }

    // ── Audio-driven transient detection ─────────────────────────────────────
    if (mode == 3 && inputBuffer.getNumChannels() > 0)
    {
        const float sens = pTransientSens_
            ? pTransientSens_->load(std::memory_order_relaxed) : 0.5f;

        const float* ch = inputBuffer.getReadPointer(0);
        const int    N  = inputBuffer.getNumSamples();

        float blockEnergy = 0.0f;
        for (int n = 0; n < N; ++n)
            blockEnergy += ch[n] * ch[n];
        blockEnergy /= std::max(N, 1);

        constexpr float kAttack  = 0.9f;
        constexpr float kRelease = 0.05f;
        energyFollower_ = (blockEnergy > energyFollower_)
            ? kAttack  * energyFollower_ + (1.0f - kAttack)  * blockEnergy
            : kRelease * energyFollower_ + (1.0f - kRelease) * blockEnergy;

        if (transientCooldown_ > 0)
            transientCooldown_ -= N;

        const float threshold = 0.3f - sens * 0.25f;
        const float ratio = (energyFollower_ > 1e-10f)
            ? blockEnergy / energyFollower_ : 0.0f;

        if (ratio > (1.0f + threshold) && transientCooldown_ <= 0)
        {
            transientFired_.store(true, std::memory_order_relaxed);
            transientCooldown_ = (int)(kTransientCooldownMs * 0.001f * (float)sampleRate_);
        }
    }
}

// ── Scheduler-thread API ──────────────────────────────────────────────────────

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

        case 2: // Euclidean — poll at step rate anchored to density
        {
            const int s = pEuclidSteps_ ? (int)pEuclidSteps_->load(std::memory_order_relaxed) : 8;
            // Default step rate: one full 8-step pattern = 1 second → 0.125s/step.
            // When density is wired via GranularEngine, the density param controls
            // the grain rate; here we use a fixed 10 grains/sec baseline divided
            // by step count to give a stable rhythmic grid.
            constexpr float kBaseGrainsPerSec = 10.0f;
            return (float)s / (kBaseGrainsPerSec * (float)std::max(s, 1));
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
    return 1.0f + rng_.nextFloat() * sprayAmt * kMaxSprayDurMult;
}

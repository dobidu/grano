#include "Color.h"
#include <cmath>

namespace
{
    constexpr float kSaturateMaxDrive = 16.0f;  // drive at amount=1.0 → 16x
    constexpr float kTiltPivotHz      = 1000.0f;
    constexpr float kTiltMaxDb        = 12.0f;
    constexpr float kDecimateMinBits  = 4.0f;
    constexpr float kDecimateMaxBits  = 24.0f;
    constexpr float kDecimateMinSrHz  = 8000.0f;
}

void Color::prepare(double sampleRate, int /*samplesPerBlock*/) noexcept
{
    sampleRate_ = sampleRate;
    tiltAlpha_ = std::exp(-juce::MathConstants<float>::twoPi
                          * kTiltPivotHz / (float)sampleRate_);
    reverb_.setSampleRate(sampleRate_);
    reset();
}

void Color::reset() noexcept
{
    tiltState_[0] = tiltState_[1] = 0.0f;
    decimateHeld_[0] = decimateHeld_[1] = 0.0f;
    decimatePhase_ = 0.0f;
    reverb_.reset();
}

void Color::setParamPointers(std::atomic<float>* enabled,
                              std::atomic<float>* saturate,
                              std::atomic<float>* decimate,
                              std::atomic<float>* tiltEq,
                              std::atomic<float>* verbMix) noexcept
{
    pEnabled_  = enabled;
    pSaturate_ = saturate;
    pDecimate_ = decimate;
    pTiltEq_   = tiltEq;
    pVerbMix_  = verbMix;
}

// ── Saturate ──────────────────────────────────────────────────────────────────
// tanh waveshaper, drive 1x → 16x, crossfaded with the dry signal by
// `amount` so engaging the stage is continuous (R-26). The shaped path is
// compensated by 1/sqrt(drive): ±12 dB worst case (quiet input boosted,
// full-scale input compressed) instead of +24 dB on quiet grains.
void Color::applySaturate(float* L, float* R, int N, float amount) noexcept
{
    if (amount < 0.001f) return;
    const float drive = 1.0f + amount * (kSaturateMaxDrive - 1.0f);
    const float comp  = 1.0f / std::sqrt(drive);
    const float wet   = amount;
    const float dry   = 1.0f - amount;
    // L == R on mono layouts — writing R would apply the shaper twice.
    for (int n = 0; n < N; ++n)
    {
        L[n] = dry * L[n] + wet * std::tanh(L[n] * drive) * comp;
        if (R != L)
            R[n] = dry * R[n] + wet * std::tanh(R[n] * drive) * comp;
    }
}

// ── Decimate ─────────────────────────────────────────────────────────────────
// amount=0 → pristine. amount=1 → 4-bit @ 8 kHz.
// Sample-rate reduction via sample-and-hold, then bit-depth quantisation.
void Color::applyDecimate(float* L, float* R, int N, float amount) noexcept
{
    if (amount < 0.001f) return;

    const float targetSr = juce::jmap(amount, (float)sampleRate_, kDecimateMinSrHz);
    const float holdLen  = (float)sampleRate_ / std::max(targetSr, 1.0f);
    const float bits     = juce::jmap(amount, kDecimateMaxBits, kDecimateMinBits);
    const float levels   = std::pow(2.0f, bits - 1.0f);

    for (int n = 0; n < N; ++n)
    {
        decimatePhase_ += 1.0f;
        if (decimatePhase_ >= holdLen)
        {
            decimatePhase_ -= holdLen;
            decimateHeld_[0] = L[n];
            decimateHeld_[1] = R[n];
        }
        L[n] = std::round(decimateHeld_[0] * levels) / levels;
        if (R != L) // aliased on mono layouts
            R[n] = std::round(decimateHeld_[1] * levels) / levels;
    }
}

// ── Tilt EQ ───────────────────────────────────────────────────────────────────
// 1-pole LPF split at kTiltPivotHz (alpha computed once in prepare()).
// low content = z[n].  high content = x[n] - z[n].
// tiltParam > 0 → boost lows / cut highs. tiltParam < 0 → cut lows / boost highs.
void Color::applyTilt(float* L, float* R, int N, float tiltParam) noexcept
{
    if (std::abs(tiltParam) < 0.001f) return;
    const float gainDb   = tiltParam * kTiltMaxDb;
    const float lowGain  = juce::Decibels::decibelsToGain( gainDb);
    const float highGain = juce::Decibels::decibelsToGain(-gainDb);
    const float a        = tiltAlpha_;
    const float oneMinA  = 1.0f - a;

    for (int n = 0; n < N; ++n)
    {
        tiltState_[0] = a * tiltState_[0] + oneMinA * L[n];
        L[n] = tiltState_[0] * lowGain + (L[n] - tiltState_[0]) * highGain;

        if (R != L) // aliased on mono layouts — cascading would filter twice
        {
            tiltState_[1] = a * tiltState_[1] + oneMinA * R[n];
            R[n] = tiltState_[1] * lowGain + (R[n] - tiltState_[1]) * highGain;
        }
    }
}

// ── Verb ─────────────────────────────────────────────────────────────────────
// juce::Reverb (Freeverb-style plate). Processes stereo in-place.
// Equal-power dry/wet law. juce::Reverb internally scales dryLevel ×2 and
// wetLevel ×3, so the parameters are pre-divided — the old dryLevel = 1.0
// meant +6 dB on the dry signal the moment verb engaged (R-15).
void Color::applyVerb(float* L, float* R, int N, float mix) noexcept
{
    if (mix < 0.001f)
    {
        verbActive_ = false;
        return;
    }
    if (!verbActive_)
    {
        reverb_.reset(); // don't release a tail frozen at the last bypass
        verbActive_ = true;
    }
    const float theta = mix * juce::MathConstants<float>::halfPi;
    juce::Reverb::Parameters p;
    p.roomSize   = 0.75f;
    p.damping    = 0.5f;
    p.wetLevel   = std::sin(theta) / 3.0f;
    p.dryLevel   = std::cos(theta) / 2.0f;
    p.width      = 1.0f;
    p.freezeMode = 0.0f;
    reverb_.setParameters(p);
    if (R != L)
        reverb_.processStereo(L, R, N);
    else // mono layout — processStereo on aliased pointers corrupts the mix
        reverb_.processMono(L, N);
}

// ── processBlock ─────────────────────────────────────────────────────────────
void Color::processBlock(juce::AudioBuffer<float>& buffer) noexcept
{
    if (pEnabled_ == nullptr)
        return;
    if (pEnabled_->load(std::memory_order_relaxed) < 0.5f)
    {
        verbActive_ = false;
        return;
    }

    const int numCh = buffer.getNumChannels();
    if (numCh < 1) return;

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getWritePointer(numCh > 1 ? 1 : 0);
    const int N = buffer.getNumSamples();

    const float satAmt  = pSaturate_ ? pSaturate_->load(std::memory_order_relaxed) : 0.0f;
    const float decAmt  = pDecimate_ ? pDecimate_->load(std::memory_order_relaxed) : 0.0f;
    const float tiltAmt = pTiltEq_   ? pTiltEq_->load(std::memory_order_relaxed)   : 0.0f;
    const float verbAmt = pVerbMix_  ? pVerbMix_->load(std::memory_order_relaxed)  : 0.0f;

    applySaturate(L, R, N, satAmt);
    applyDecimate(L, R, N, decAmt);
    applyTilt    (L, R, N, tiltAmt);
    applyVerb    (L, R, N, verbAmt);
}

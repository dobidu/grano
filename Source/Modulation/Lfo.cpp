#include "Lfo.h"
#include <cmath>
#include <algorithm>

Lfo::Lfo() noexcept
{
    for (auto& pt : drawable_)
        pt.store(0.0f, std::memory_order_relaxed);
}

void Lfo::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
    reset();
}

void Lfo::reset() noexcept
{
    phase_            = 0.0;
    lastValue_        = 0.0f;
    sAndHHeld_        = 0.0f;
    prevPhaseForSH_   = 0.0f;
    randCurrent_      = 0.0f;
    randNext_         = rng_.nextFloat() * 2.0f - 1.0f;
    prevPhaseForRand_ = 0.0f;
    syncRateHz_       = 1.0f;
}

void Lfo::setParamPointers(std::atomic<float>* rate,
                            std::atomic<float>* waveform,
                            std::atomic<float>* sync,
                            std::atomic<float>* phase,
                            std::atomic<float>* depth) noexcept
{
    pRate_     = rate;
    pWaveform_ = waveform;
    pSync_     = sync;
    pPhase_    = phase;
    pDepth_    = depth;
}

void Lfo::setDrawablePoint(int index, float value) noexcept
{
    index = juce::jlimit(0, kDrawablePoints - 1, index);
    value = juce::jlimit(-1.0f, 1.0f, value);
    drawable_[index].store(value, std::memory_order_relaxed);
}

float Lfo::getDrawablePoint(int index) const noexcept
{
    index = juce::jlimit(0, kDrawablePoints - 1, index);
    return drawable_[index].load(std::memory_order_relaxed);
}

void Lfo::processBlock(int /*numSamples*/, double bpm) noexcept
{
    if (pSync_ && pSync_->load(std::memory_order_relaxed) >= 0.5f && bpm > 0.0)
        syncRateHz_ = static_cast<float>(bpm / 60.0);
}

float Lfo::advanceSample() noexcept
{
    const float rateHz  = pRate_    ? pRate_->load(std::memory_order_relaxed)     : 1.0f;
    const int   wfIdx   = pWaveform_? (int)pWaveform_->load(std::memory_order_relaxed) : 0;
    const bool  synced  = pSync_    ? pSync_->load(std::memory_order_relaxed) >= 0.5f : false;
    const float phOff   = pPhase_   ? pPhase_->load(std::memory_order_relaxed)    : 0.0f;
    const float depth   = pDepth_   ? pDepth_->load(std::memory_order_relaxed)    : 0.0f;

    const float effectiveRate = std::max(0.01f,
        (synced ? syncRateHz_ : rateHz) + rateModOffset_.load(std::memory_order_relaxed));
    const double increment = effectiveRate / sampleRate_;

    // Apply phase offset (degrees → normalized)
    const float normPhase = static_cast<float>(
        std::fmod(phase_ + phOff / 360.0, 1.0));

    const auto wf = static_cast<Waveform>(juce::jlimit(0, 6, wfIdx));
    const float sample = computeSample(normPhase, wf);

    lastValue_ = sample * depth;
    phase_ = std::fmod(phase_ + increment, 1.0);
    return lastValue_;
}

float Lfo::computeSample(float p, Waveform wf) noexcept
{
    switch (wf)
    {
        case Waveform::Sine:
            return std::sin(p * juce::MathConstants<float>::twoPi);

        case Waveform::Triangle:
            return p < 0.5f ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);

        case Waveform::Saw:
            return 2.0f * p - 1.0f;

        case Waveform::Square:
            return p < 0.5f ? 1.0f : -1.0f;

        case Waveform::Random:
        {
            // On phase wrap (p crossed zero), shift and sample new value
            if (p < prevPhaseForRand_)
            {
                randCurrent_ = randNext_;
                randNext_ = rng_.nextFloat() * 2.0f - 1.0f;
            }
            prevPhaseForRand_ = p;
            // Linear interpolation across cycle
            return randCurrent_ + p * (randNext_ - randCurrent_);
        }

        case Waveform::SAndH:
        {
            // On phase wrap, sample a new held value
            if (p < prevPhaseForSH_)
                sAndHHeld_ = rng_.nextFloat() * 2.0f - 1.0f;
            prevPhaseForSH_ = p;
            return sAndHHeld_;
        }

        case Waveform::Drawable:
            return sampleDrawable(p);

        default:
            return 0.0f;
    }
}

float Lfo::sampleDrawable(float normPhase) const noexcept
{
    const float fidx = normPhase * static_cast<float>(kDrawablePoints);
    const int   i0   = static_cast<int>(fidx) % kDrawablePoints;
    const int   i1   = (i0 + 1) % kDrawablePoints;
    const float frac = fidx - std::floor(fidx);
    const float v0   = drawable_[i0].load(std::memory_order_relaxed);
    const float v1   = drawable_[i1].load(std::memory_order_relaxed);
    return v0 + frac * (v1 - v0);
}

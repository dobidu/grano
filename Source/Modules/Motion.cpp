#include "Motion.h"

namespace
{
    constexpr float kWowMaxCents     = 50.0f;
    constexpr float kFlutterMaxCents = 10.0f;
    constexpr float kDriftMaxCents   = 20.0f;
    constexpr float kTwoPi           = 6.28318530718f;
    constexpr double kDriftRerollMinSec = 0.5;
    constexpr double kDriftRerollMaxSec = 2.0;
    constexpr double kDriftTauSec       = 0.6;
    constexpr float  kCrackleSplitHz    = 800.0f;
}

void Motion::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
    splitCoeff_ = std::exp(-kTwoPi * kCrackleSplitHz / (float)sampleRate);
    reset();
}

void Motion::reset() noexcept
{
    wowPhase_ = flutterPhase_ = 0.0f;
    driftCurrent_ = driftTarget_ = 0.0f;
    driftTimerSec_ = 0.0;
    for (auto& ch : pink_)
        for (auto& b : ch)
            b = 0.0f;
    filterState_[0] = filterState_[1] = 0.0f;
    pitchModSt_.store(0.0f, std::memory_order_relaxed);
}

void Motion::setParamPointers(std::atomic<float>* enabled,
                               std::atomic<float>* wowDepth,  std::atomic<float>* wowRate,
                               std::atomic<float>* flutDepth, std::atomic<float>* flutRate,
                               std::atomic<float>* drift,
                               std::atomic<float>* crackLevel,
                               std::atomic<float>* crackColor) noexcept
{
    pEnabled_    = enabled;
    pWowDepth_   = wowDepth;  pWowRate_   = wowRate;
    pFlutDepth_  = flutDepth; pFlutRate_  = flutRate;
    pDrift_      = drift;
    pCrackLevel_ = crackLevel;
    pCrackColor_ = crackColor;
}

float Motion::nextPink(int ch) noexcept
{
    float* b = pink_[ch & 1];
    const float white = rng_.nextFloat() * 2.0f - 1.0f;
    b[0] = 0.99886f * b[0] + white * 0.0555179f;
    b[1] = 0.99332f * b[1] + white * 0.0750759f;
    b[2] = 0.96900f * b[2] + white * 0.1538520f;
    b[3] = 0.86650f * b[3] + white * 0.3104856f;
    b[4] = 0.55000f * b[4] + white * 0.5329522f;
    b[5] = -0.7616f * b[5] - white * 0.0168980f;
    const float pink = (b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + white * 0.5362f) * 0.11f;
    b[6] = white * 0.115926f;
    return juce::jlimit(-1.0f, 1.0f, pink);
}

void Motion::updatePitchMod(int numSamples) noexcept
{
    const float sr         = (float)sampleRate_;
    const float wowDepth   = pWowDepth_  ? pWowDepth_->load(std::memory_order_relaxed)  : 0.0f;
    const float wowRate    = pWowRate_   ? pWowRate_->load(std::memory_order_relaxed)   : 0.5f;
    const float flutDepth  = pFlutDepth_ ? pFlutDepth_->load(std::memory_order_relaxed) : 0.0f;
    const float flutRate   = pFlutRate_  ? pFlutRate_->load(std::memory_order_relaxed)  : 8.0f;
    const float driftAmt   = pDrift_     ? pDrift_->load(std::memory_order_relaxed)     : 0.0f;

    // Wow — advance phase by block, sample LFO at end of block
    wowPhase_ = std::fmod(wowPhase_ + kTwoPi * wowRate  / sr * (float)numSamples, kTwoPi);
    const float wowCents = std::sin(wowPhase_) * wowDepth * kWowMaxCents;

    // Flutter — sine + 10% white noise
    flutterPhase_ = std::fmod(flutterPhase_ + kTwoPi * flutRate / sr * (float)numSamples, kTwoPi);
    const float flutCents = (0.9f * std::sin(flutterPhase_)
                             + 0.1f * (rng_.nextFloat() * 2.0f - 1.0f))
                            * flutDepth * kFlutterMaxCents;

    // Drift — smoothed random walk toward a target re-rolled every
    // 0.5–2 s. Timed in seconds (not blocks) so it behaves the same at any
    // host buffer size (R-18); depth applied at the output.
    const double blockSec = (double)numSamples / sampleRate_;
    driftTimerSec_ -= blockSec;
    if (driftTimerSec_ <= 0.0)
    {
        driftTimerSec_ = kDriftRerollMinSec
            + (double)rng_.nextFloat() * (kDriftRerollMaxSec - kDriftRerollMinSec);
        driftTarget_ = rng_.nextFloat() * 2.0f - 1.0f;
    }
    const float coef = (float)(1.0 - std::exp(-blockSec / kDriftTauSec));
    driftCurrent_ += (driftTarget_ - driftCurrent_) * coef;
    const float driftCents = driftCurrent_ * driftAmt * kDriftMaxCents;

    // Publish in semitones (cents / 100)
    pitchModSt_.store((wowCents + flutCents + driftCents) / 100.0f,
                      std::memory_order_relaxed);
}

void Motion::processBlock(juce::AudioBuffer<float>& buffer) noexcept
{
    if (pEnabled_ == nullptr)
        return;

    const bool enabled = pEnabled_->load(std::memory_order_relaxed) > 0.5f;
    if (!enabled)
    {
        pitchModSt_.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const int N = buffer.getNumSamples();

    updatePitchMod(N);

    // Crackle — filtered pink noise added to output
    const float crackLevelDb = pCrackLevel_ ? pCrackLevel_->load(std::memory_order_relaxed) : -60.0f;
    if (crackLevelDb <= -59.9f)
        return;

    const float gain       = juce::Decibels::decibelsToGain(crackLevelDb, -60.0f);
    const float crackColor = juce::jlimit(0.0f, 1.0f,
        pCrackColor_ ? pCrackColor_->load(std::memory_order_relaxed) : 0.5f);
    // Fixed 800 Hz split; color crossfades LP (warm) → HP (bright) with
    // equal-power weights. Pink noise carries similar power either side of
    // the split, so loudness stays roughly constant across the knob (R-17)
    // — the old design moved the cutoff too, muting crackle at color = 1.
    const float wLow  = std::cos(crackColor * kTwoPi * 0.25f);
    const float wHigh = std::sin(crackColor * kTwoPi * 0.25f);
    const float a     = splitCoeff_;

    const int numCh = std::min(buffer.getNumChannels(), 2);
    for (int ch = 0; ch < numCh; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        if (ch > 0 && data == buffer.getWritePointer(0))
            break; // aliased mono layout
        float& lp = filterState_[ch];
        for (int n = 0; n < N; ++n)
        {
            const float pink = nextPink(ch);
            lp = a * lp + (1.0f - a) * pink;
            data[n] += (wLow * lp + wHigh * (pink - lp)) * gain;
        }
    }
}

#include "Motion.h"

namespace
{
    constexpr float kWowMaxCents     = 50.0f;
    constexpr float kFlutterMaxCents = 10.0f;
    constexpr float kDriftMaxCents   = 20.0f;
    constexpr float kTwoPi           = 6.28318530718f;
    constexpr int   kDriftBlocksMin  = 200;
    constexpr int   kDriftBlocksMax  = 800;
}

void Motion::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
    reset();
}

void Motion::reset() noexcept
{
    wowPhase_ = flutterPhase_ = 0.0f;
    driftCurrent_ = driftTarget_ = 0.0f;
    driftCounter_ = kDriftBlocksMin;
    b0_ = b1_ = b2_ = b3_ = b4_ = b5_ = b6_ = 0.0f;
    filterState_ = 0.0f;
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

float Motion::nextPink() noexcept
{
    const float white = rng_.nextFloat() * 2.0f - 1.0f;
    b0_ = 0.99886f * b0_ + white * 0.0555179f;
    b1_ = 0.99332f * b1_ + white * 0.0750759f;
    b2_ = 0.96900f * b2_ + white * 0.1538520f;
    b3_ = 0.86650f * b3_ + white * 0.3104856f;
    b4_ = 0.55000f * b4_ + white * 0.5329522f;
    b5_ = -0.7616f * b5_ - white * 0.0168980f;
    const float pink = (b0_ + b1_ + b2_ + b3_ + b4_ + b5_ + b6_ + white * 0.5362f) * 0.11f;
    b6_ = white * 0.115926f;
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

    // Drift — random walk toward slowly-changing target
    if (--driftCounter_ <= 0)
    {
        driftCounter_ = kDriftBlocksMin + rng_.nextInt(kDriftBlocksMax - kDriftBlocksMin);
        driftTarget_  = (rng_.nextFloat() * 2.0f - 1.0f) * driftAmt * kDriftMaxCents;
    }
    driftCurrent_ += (driftTarget_ - driftCurrent_) * 0.0005f;

    // Publish in semitones (cents / 100)
    pitchModSt_.store((wowCents + flutCents + driftCurrent_) / 100.0f,
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

    const float gain      = juce::Decibels::decibelsToGain(crackLevelDb, -60.0f);
    const float crackColor = pCrackColor_ ? pCrackColor_->load(std::memory_order_relaxed) : 0.5f;
    // color=0 → LPF (warm), color=1 → HPF (bright)
    const float lpfCoeff  = 1.0f - crackColor * 0.97f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int n = 0; n < N; ++n)
        {
            const float pink    = nextPink();
            filterState_        = lpfCoeff * filterState_ + (1.0f - lpfCoeff) * pink;
            const float colored = juce::jmap(crackColor, filterState_, pink - filterState_);
            data[n] += colored * gain;
        }
    }
}

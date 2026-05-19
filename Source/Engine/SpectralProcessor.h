#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <algorithm>

// SpectralProcessor — FFT-based source transformation.
// processSource() runs on a worker thread (NOT audio thread).
// Outputs 2048-sample grain source via getReadPointer()/getNumSamples().
class SpectralProcessor
{
public:
    static constexpr int kFFTOrder = 11;
    static constexpr int kFFTSize  = 1 << kFFTOrder; // 2048

    enum class Mode : int { Freeze = 0, Blur = 1 };

    SpectralProcessor();

    // Process source into spectral output. Call from worker/message thread.
    // srcLen should be >= kFFTSize (padded with zeros otherwise).
    void processSource(const float* src, int srcLen,
                       Mode mode, float blurAmount) noexcept;

    const float* getReadPointer() const noexcept { return output_.data(); }
    int          getNumSamples()  const noexcept { return kFFTSize; }

private:
    juce::dsp::FFT fft_;
    std::array<float, kFFTSize * 2> fftBuf_{}; // complex interleaved
    std::array<float, kFFTSize>     window_{};
    std::array<float, kFFTSize>     output_{};
    juce::Random                    rng_;
};

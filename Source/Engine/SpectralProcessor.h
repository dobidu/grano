#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <algorithm>

// SpectralProcessor — FFT-based source transformation.
// processSource() runs on the message thread (NOT audio thread).
// Outputs 2048-sample grain source via getReadPointer()/getNumSamples().
class SpectralProcessor
{
public:
    static constexpr int kFFTOrder = 11;
    static constexpr int kFFTSize  = 1 << kFFTOrder; // 2048

    enum class Mode : int { Freeze = 0, Blur = 1 };

    SpectralProcessor();

    // Spectral processing is offline on the source: it adds NO delay to the
    // audio path, so the plugin reports 0 latency for it (R-10).
    // Process source into spectral output. Call from worker/message thread.
    // srcLen should be >= kFFTSize (padded with zeros otherwise).
    void processSource(const float* src, int srcLen,
                       Mode mode, float blurAmount) noexcept;

    // getNumSamples() excludes the +2 guard samples (renderGrain contract:
    // interpolation may read source[len] and source[len+1]).
    const float* getReadPointer() const noexcept { return output_.data(); }
    int          getNumSamples()  const noexcept { return kFFTSize; }

private:
    juce::dsp::FFT fft_;
    std::array<float, kFFTSize * 2> fftBuf_{}; // complex interleaved
    // +2 zero guard samples past kFFTSize for linear-interpolation safety
    // (processSource only writes [0, kFFTSize); guards stay zero).
    std::array<float, kFFTSize + 2> output_{};
    juce::Random                    rng_;
};

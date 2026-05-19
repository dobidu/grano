#include "SpectralProcessor.h"

SpectralProcessor::SpectralProcessor()
    : fft_(kFFTOrder)
{
    // Build Hann window
    for (int i = 0; i < kFFTSize; ++i)
        window_[i] = 0.5f * (1.0f - std::cos(6.28318f * (float)i / (float)(kFFTSize - 1)));
}

void SpectralProcessor::processSource(const float* src, int srcLen,
                                      Mode mode, float blurAmount) noexcept
{
    // Fill FFT input with windowed source (first kFFTSize samples, padded with zeros).
    const int readLen = std::min(srcLen, kFFTSize);
    for (int i = 0; i < readLen; ++i)
        fftBuf_[i] = src[i] * window_[i];
    for (int i = readLen; i < kFFTSize; ++i)
        fftBuf_[i] = 0.0f;
    for (int i = kFFTSize; i < kFFTSize * 2; ++i)
        fftBuf_[i] = 0.0f;

    fft_.performRealOnlyForwardTransform(fftBuf_.data(), true);

    const int bins = kFFTSize / 2 + 1;
    for (int k = 1; k < bins; ++k)
    {
        const float re  = fftBuf_[2 * k];
        const float im  = fftBuf_[2 * k + 1];
        const float mag = std::sqrt(re * re + im * im);

        float newPhase;
        float newMag = mag;

        if (mode == Mode::Freeze)
        {
            newPhase = 0.0f;
        }
        else // Blur
        {
            newPhase = rng_.nextFloat() * 6.28318f * blurAmount;
            newMag = mag * (1.0f - 0.3f * blurAmount + 0.3f * blurAmount * rng_.nextFloat());
        }

        fftBuf_[2 * k]     = newMag * std::cos(newPhase);
        fftBuf_[2 * k + 1] = newMag * std::sin(newPhase);
    }
    // Mirror negative frequencies for real output (conjugate symmetry)
    for (int k = bins; k < kFFTSize; ++k)
    {
        fftBuf_[2 * k]     =  fftBuf_[2 * (kFFTSize - k)];
        fftBuf_[2 * k + 1] = -fftBuf_[2 * (kFFTSize - k) + 1];
    }

    fft_.performRealOnlyInverseTransform(fftBuf_.data());

    // JUCE's performRealOnlyInverseTransform normalises by 1/N internally;
    // no manual /kFFTSize needed here.
    for (int i = 0; i < kFFTSize; ++i)
        output_[i] = fftBuf_[i];
}

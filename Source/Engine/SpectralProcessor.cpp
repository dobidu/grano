#include "SpectralProcessor.h"

SpectralProcessor::SpectralProcessor()
    : fft_(kFFTOrder)
{
}

void SpectralProcessor::processSource(const float* src, int srcLen,
                                      Mode mode, float blurAmount) noexcept
{
    // One 2048-sample frame, unwindowed: the output is a looping grain
    // source, and skipping the window keeps level (Parseval) and lets
    // blur = 0 reproduce the source frame exactly.
    const int readLen = std::min(srcLen, kFFTSize);
    for (int i = 0; i < readLen; ++i)
        fftBuf_[(size_t)i] = src[i];
    for (int i = readLen; i < kFFTSize * 2; ++i)
        fftBuf_[(size_t)i] = 0.0f;

    fft_.performRealOnlyForwardTransform(fftBuf_.data(), true);

    // Freeze: keep every bin's magnitude, randomise its phase — the classic
    // spectral freeze (a sustained, stationary texture). Zero phase (the old
    // behaviour) collapses the frame into a single click at n = 0 (R-07).
    // Seeded so the same source always freezes to the same texture.
    // Blur: offset the ORIGINAL phase by up to ±pi*blur and jitter the
    // magnitude by blur — blur = 0 is the dry frame (R-08).
    juce::Random freezeRng(0x6772616eLL);
    const float blur = std::clamp(blurAmount, 0.0f, 1.0f);
    constexpr float kTwoPi = 6.283185307f;

    const int nyquist = kFFTSize / 2;
    for (int k = 1; k <= nyquist; ++k)
    {
        const float re  = fftBuf_[(size_t)(2 * k)];
        const float im  = fftBuf_[(size_t)(2 * k + 1)];
        float mag   = std::sqrt(re * re + im * im);
        float phase;

        if (mode == Mode::Freeze)
        {
            phase = freezeRng.nextFloat() * kTwoPi;
        }
        else
        {
            phase = std::atan2(im, re) + (rng_.nextFloat() - 0.5f) * kTwoPi * blur;
            mag  *= 1.0f - 0.3f * blur + 0.3f * blur * rng_.nextFloat();
        }

        if (k == nyquist)
        {
            // The Nyquist bin of a real signal is real: keep it on the axis.
            fftBuf_[(size_t)(2 * k)]     = (std::cos(phase) >= 0.0f ? mag : -mag);
            fftBuf_[(size_t)(2 * k + 1)] = 0.0f;
        }
        else
        {
            fftBuf_[(size_t)(2 * k)]     = mag * std::cos(phase);
            fftBuf_[(size_t)(2 * k + 1)] = mag * std::sin(phase);
        }
    }
    // Mirror negative frequencies for real output (conjugate symmetry)
    for (int k = nyquist + 1; k < kFFTSize; ++k)
    {
        fftBuf_[(size_t)(2 * k)]     =  fftBuf_[(size_t)(2 * (kFFTSize - k))];
        fftBuf_[(size_t)(2 * k + 1)] = -fftBuf_[(size_t)(2 * (kFFTSize - k) + 1)];
    }

    fft_.performRealOnlyInverseTransform(fftBuf_.data());

    // JUCE's performRealOnlyInverseTransform normalises by 1/N internally;
    // no manual /kFFTSize needed here.
    for (int i = 0; i < kFFTSize; ++i)
        output_[(size_t)i] = fftBuf_[(size_t)i];
}

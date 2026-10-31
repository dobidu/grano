#pragma once

#include "EnvelopeShapes.h"
#include <type_traits>

// Grain — single granular synthesis unit descriptor.
//
// Plain data struct; trivially copyable so the audio thread can copy grain
// state without virtual dispatch or heap involvement. The source pointer is
// non-owning — the caller (SampleBuffer, F2+) guarantees the buffer outlives
// the grain. No constructor or destructor: aggregate initialisation only.
//
// Playback position is an integer sample counter (`elapsed`), not a float
// phase accumulator: float accumulation of 1/len drifts (pitch error) and a
// float read position loses sub-sample precision on long sources (R-05).
struct Grain
{
    const float*  source        = nullptr;           // non-owning pointer into sample buffer
    int           startPos      = 0;                 // sample index within source
    int           sourceLen     = 0;                 // audio samples in source (guards excluded)
    int           lengthSamples = 0;                 // grain duration in output samples
    float         pitchRatio    = 1.0f;              // 1.0 = original pitch; >1 = up, <1 = down
    float         pan           = 0.0f;              // -1 (L) .. 0 (centre) .. +1 (R)
    EnvelopeShape shape         = EnvelopeShape::Hann;
    int           elapsed       = 0;                 // output samples already rendered
    bool          reversed      = false;             // true: reads end→start (F4c Pattern)
    int           startDelay    = 0;                 // output samples to wait before sounding
    float         gain          = 1.0f;              // per-grain level (overlap normalisation)
    int           wrapLen       = 0;                 // >0: source is a ring of this length (feedback)

    // Normalised envelope phase [0..1].
    float phase() const noexcept
    {
        return lengthSamples > 0 ? (float)elapsed / (float)lengthSamples : 1.0f;
    }
};

static_assert(std::is_trivially_copyable_v<Grain>,
              "Grain must be trivially copyable for safe audio-thread use");

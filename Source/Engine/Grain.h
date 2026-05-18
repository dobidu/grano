#pragma once

#include "EnvelopeShapes.h"
#include <type_traits>

// Grain — single granular synthesis unit descriptor.
//
// Plain data struct; trivially copyable so the audio thread can copy grain
// state without virtual dispatch or heap involvement. The source pointer is
// non-owning — the caller (SampleBuffer, F2+) guarantees the buffer outlives
// the grain. No constructor or destructor: aggregate initialisation only.
struct Grain
{
    const float*  source        = nullptr;           // non-owning pointer into sample buffer
    int           startPos      = 0;                 // sample index within source
    int           lengthSamples = 0;                 // grain duration in samples
    float         pitchRatio    = 1.0f;              // 1.0 = original pitch; >1 = up, <1 = down
    float         pan           = 0.0f;              // -1 (L) .. 0 (centre) .. +1 (R)
    EnvelopeShape shape         = EnvelopeShape::Hann;
    float         currentPhase  = 0.0f;              // 0..1 normalised playback position
    bool          reversed      = false;             // true: reads end→start (F4c Pattern)
};

static_assert(std::is_trivially_copyable_v<Grain>,
              "Grain must be trivially copyable for safe audio-thread use");

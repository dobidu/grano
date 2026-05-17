#pragma once

enum class EnvelopeShape { Hann, Tukey, Gaussian, Triangle, Rectangle };

// Apply a window envelope at normalised phase [0, 1] → amplitude [0, 1].
// Pure function — no state, no allocation, noexcept. Safe on the audio thread.
float applyEnvelope(EnvelopeShape shape, float phase) noexcept;

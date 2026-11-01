#pragma once
#include <juce_core/juce_core.h>

// ParamFormat — display text for every Grano parameter, owned by the UI.
//
// Parameter IDs, ranges and choices are fixed by Parameters.cpp; only how a
// value is *shown* lives here: units ("120 ms", "−6.0 dB", "+7.0 st",
// "45 %"), full names for the command line, short hints for tooltips and the
// status bar, and the mod-matrix destination → parameter ID table.
// Pure functions: unit-tested in Tests/test_ui_format.cpp.

namespace ParamFormat
{
    // Value text with unit for a real (denormalised) parameter value.
    // Choice / bool parameters return their choice label / "on"/"off".
    juce::String valueText(const juce::String& paramID, float value);

    // Number and unit split for two-font rendering ("120", "ms"). unit is
    // empty when the text has none (e.g. "flat", "Hann", "4").
    struct Parts { juce::String number, unit; };
    Parts split(const juce::String& text);

    // Full human name for the command line ("Position Jitter", "LFO 1 Rate",
    // "Mod 3 Amount").
    juce::String fullName(const juce::String& paramID);

    // One-line explanation used as tooltip and status hint.
    juce::String hint(const juce::String& paramID);

    // Mod matrix destination index (Parameters.cpp kModDests order) → the
    // parameter ID it modulates; empty for 0 ("None") or out of range.
    juce::String modDestParamID(int destIndex);
    inline constexpr int kNumModDests = 29;
}

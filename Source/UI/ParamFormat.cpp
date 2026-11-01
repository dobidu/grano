#include "ParamFormat.h"
#include <cmath>
#include <map>

namespace
{
    const juce::String kMinus = juce::String::fromUTF8("\xe2\x88\x92"); // U+2212

    juce::String pct(float v) { return juce::String(juce::roundToInt(v * 100.0f)) + " %"; }

    // Signed number with true minus sign; zero carries no sign.
    juce::String signedNum(float v, int decimals, const juce::String& unit)
    {
        const auto mag = juce::String(std::abs(v), decimals);
        if (mag == juce::String(0.0f, decimals) || std::abs(v) < 1.0e-9f)
            return juce::String(0.0f, decimals) + unit;
        return (v > 0.0f ? "+" : kMinus) + mag + unit;
    }

    juce::String hz(float v)
    {
        if (v >= 1000.0f) return juce::String(v / 1000.0f, 2) + " kHz";
        if (v >= 100.0f)  return juce::String(juce::roundToInt(v)) + " Hz";
        if (v >= 10.0f)   return juce::String(v, 1) + " Hz";
        return juce::String(v, 2) + " Hz";
    }

    juce::String ms(float v) { return (v < 10.0f ? juce::String(v, 1) : juce::String(juce::roundToInt(v))) + " ms"; }
    juce::String perSec(float v) { return (v < 10.0f ? juce::String(v, 1) : juce::String(juce::roundToInt(v))) + " g/s"; }
    juce::String integer(float v) { return juce::String(juce::roundToInt(v)); }

    juce::String choice(const juce::StringArray& c, float v)
    {
        return c[juce::jlimit(0, c.size() - 1, juce::roundToInt(v))];
    }

    const juce::StringArray kModDestIDs{
        "", "grainSize", "density", "position", "positionJitter", "pitchShift",
        "stereoSpread", "masterVolume",
        "wowDepth", "wowRate", "flutterDepth", "flutterRate", "driftAmount",
        "crackleLevel", "crackleColor",
        "saturate", "decimate", "tiltEq", "verbMix",
        "probability", "reverseProb", "spray", "transientSensitivity",
        "lfo1Rate", "lfo1Phase", "lfo1Depth",
        "lfo2Rate", "lfo2Phase", "lfo2Depth"
    };
    const juce::StringArray kModDests{
        "None", "Grain Size", "Density", "Position", "Pos Jitter", "Pitch Shift",
        "Stereo Spread", "Master Vol",
        "Wow Depth", "Wow Rate", "Flutter Depth", "Flutter Rate", "Drift",
        "Crackle Lvl", "Crackle Color",
        "Saturate", "Decimate", "Tilt EQ", "Verb Mix",
        "Probability", "Reverse Prob", "Spray", "Transient Sens",
        "LFO1 Rate", "LFO1 Phase", "LFO1 Depth",
        "LFO2 Rate", "LFO2 Phase", "LFO2 Depth"
    };

    // "slot3Amount" → (3, "Amount"); index 0 when not a mod-matrix ID.
    std::pair<int, juce::String> matrixSlot(const juce::String& id)
    {
        if (id.startsWith("slot") && id.length() > 5 && juce::CharacterFunctions::isDigit(id[4]))
        {
            const auto rest = id.substring(5);
            if (rest == "Source" || rest == "Dest" || rest == "Amount")
                return { id.substring(4, 5).getIntValue(), rest };
        }
        return { 0, {} };
    }

    // "slot2Weight" → 2 (0-based sample bank slot), -1 otherwise.
    int weightSlot(const juce::String& id)
    {
        if (id.startsWith("slot") && id.endsWith("Weight") && id.length() == 11)
            return id.substring(4, 5).getIntValue();
        return -1;
    }
}

juce::String ParamFormat::valueText(const juce::String& id, float v)
{
    if (id == "grainSize")      return ms(v);
    if (id == "density")        return perSec(v);
    if (id == "pitchShift")     return signedNum(v, 1, " st");
    if (id == "masterVolume")   return v <= -59.95f ? kMinus + juce::String::fromUTF8("\xe2\x88\x9e dB")
                                                    : signedNum(v, 1, " dB");
    if (id == "crackleLevel")   return v <= -59.5f ? juce::String("off")
                                                   : signedNum((float)juce::roundToInt(v), 0, " dB");
    if (id == "wowRate" || id == "flutterRate" || id == "lfo1Rate" || id == "lfo2Rate")
        return hz(v);
    if (id == "lfo1Phase" || id == "lfo2Phase")
        return juce::String(juce::roundToInt(v)) + juce::String::fromUTF8("\xc2\xb0");
    if (id == "tiltEq")
    {
        if (std::abs(v) < 0.005f) return "flat";
        return (v < 0.0f ? "dark " : "bright ") + juce::String(juce::roundToInt(std::abs(v) * 100.0f));
    }
    if (id == "euclidPulses" || id == "euclidSteps" || id == "euclidRotation" || id == "subGrainDepth")
        return integer(v);

    if (id == "loop" || id.endsWith("Enabled") || id.endsWith("Sync"))
        return v > 0.5f ? "on" : "off";

    if (id == "stochasticDist")
        return choice({ "Uniform", "Gaussian", "Poisson", "Exponential", "Pareto", "1/f" }, v);
    if (id == "spectralMode")   return choice({ "Freeze", "Blur" }, v);
    if (id == "envelopeShape")  return choice({ "Hann", "Tukey", "Gaussian", "Triangle", "Rectangle" }, v);
    if (id == "triggerMode")    return choice({ "Free", "Sync", "Euclid", "Audio" }, v);
    if (id == "syncDivision")
        return choice({ "1/4", "1/8", "1/16", "1/32", "1/64", "1/4T", "1/8T", "1/16T", "1/4D", "1/8D" }, v);
    if (id == "quantizeScale")
        return choice({ "Chromatic", "Major", "Minor", "Dorian", "Phrygian", "Lydian",
                        "Mixolydian", "Pent Maj", "Pent Min", "Whole Tone", "Octatonic" }, v);
    if (id == "lfo1Waveform" || id == "lfo2Waveform")
        return choice({ "Sine", "Triangle", "Saw", "Square", "Random", "S&H", "Drawable" }, v);

    const auto [slot, part] = matrixSlot(id);
    if (slot > 0)
    {
        if (part == "Source") return choice({ "None", "LFO 1", "LFO 2" }, v);
        if (part == "Dest")   return choice(kModDests, v);
        return signedNum((float)juce::roundToInt(v * 100.0f), 0, " %");
    }

    // Everything else is a 0..1 amount shown as percent.
    return pct(v);
}

ParamFormat::Parts ParamFormat::split(const juce::String& text)
{
    static const juce::StringArray units{ "%", "ms", "g/s", "st", "dB", "Hz", "kHz" };
    const int sp = text.lastIndexOfChar(' ');
    if (sp > 0)
    {
        const auto unit = text.substring(sp + 1);
        const auto num  = text.substring(0, sp);
        if (units.contains(unit) && num.isNotEmpty()
            && (juce::CharacterFunctions::isDigit(num.getLastCharacter())
                || num.endsWith(juce::String::fromUTF8("\xe2\x88\x9e"))))
            return { num, unit };
    }
    return { text, {} };
}

juce::String ParamFormat::fullName(const juce::String& id)
{
    static const std::map<juce::String, juce::String> names{
        { "grainSize", "Grain Size" }, { "density", "Density" }, { "position", "Position" },
        { "positionJitter", "Position Jitter" }, { "pitchShift", "Pitch Shift" },
        { "stereoSpread", "Stereo Spread" }, { "masterVolume", "Master Volume" }, { "loop", "Loop" },
        { "motionEnabled", "Motion" }, { "wowDepth", "Wow Depth" }, { "wowRate", "Wow Rate" },
        { "flutterDepth", "Flutter Depth" }, { "flutterRate", "Flutter Rate" }, { "driftAmount", "Drift" },
        { "crackleLevel", "Crackle Level" }, { "crackleColor", "Crackle Tone" },
        { "colorEnabled", "Color" }, { "saturate", "Saturate" }, { "decimate", "Decimate" },
        { "tiltEq", "Tilt EQ" }, { "verbMix", "Verb Mix" },
        { "patternEnabled", "Pattern" }, { "triggerMode", "Trigger Mode" }, { "syncDivision", "Sync Division" },
        { "euclidPulses", "Euclid Pulses" }, { "euclidSteps", "Euclid Steps" }, { "euclidRotation", "Euclid Rotation" },
        { "transientSensitivity", "Transient Sensitivity" }, { "probability", "Probability" },
        { "reverseProb", "Reverse Probability" }, { "quantizeScale", "Quantize Scale" }, { "spray", "Spray" },
        { "subGrainDepth", "Sub-grain Depth" }, { "stochasticDist", "Timing Distribution" },
        { "feedbackEnabled", "Feedback" }, { "feedbackGain", "Feedback Gain" }, { "feedbackDamp", "Feedback Damp" },
        { "spectralEnabled", "Spectral" }, { "spectralMode", "Spectral Mode" }, { "spectralBlurAmount", "Spectral Blur" },
        { "envelopeShape", "Envelope Shape" },
    };
    if (auto it = names.find(id); it != names.end())
        return it->second;

    if (id.startsWith("lfo") && id.length() > 4)
    {
        const auto rest = id.substring(4);
        return "LFO " + id.substring(3, 4) + " " + (rest == "Waveform" ? juce::String("Wave") : rest);
    }
    if (const auto [slot, part] = matrixSlot(id); slot > 0)
        return "Mod " + juce::String(slot) + " " + (part == "Dest" ? juce::String("Destination") : part);
    if (const int s = weightSlot(id); s >= 0)
        return "Slot " + juce::String(s + 1) + " Weight";
    return id;
}

juce::String ParamFormat::hint(const juce::String& id)
{
    static const std::map<juce::String, juce::String> hints{
        { "grainSize", "Grain Size: length of each grain, 5-500 ms. Short = buzzy, long = smeared." },
        { "density", "Density: grains per second. Overlap is level-compensated." },
        { "position", "Position: where in the sample grains read from. Also click the monitor." },
        { "positionJitter", "Position Jitter: random spread of each grain's read point around Position." },
        { "pitchShift", "Pitch Shift: +/-24 semitones per grain." },
        { "stereoSpread", "Stereo Spread: random pan width. 0 = mono, 100 % = hard left/right." },
        { "masterVolume", "Master Volume: output level after all modules. -60 dB is silence." },
        { "loop", "Loop: sweeps the read position through the sample once per sample length." },
        { "motionEnabled", "Motion on/off: tape wow, flutter, drift and vinyl crackle." },
        { "wowDepth", "Wow Depth: slow tape pitch drift." },
        { "wowRate", "Wow Rate: speed of the wow drift, 0.1-2 Hz." },
        { "flutterDepth", "Flutter Depth: fast tape pitch wobble." },
        { "flutterRate", "Flutter Rate: 3-20 Hz." },
        { "driftAmount", "Drift: slow random wander of pitch and level." },
        { "crackleLevel", "Crackle Level: vinyl crackle noise, -60 dB = off." },
        { "crackleColor", "Crackle Tone: dark rumble to bright clicks." },
        { "colorEnabled", "Color on/off: saturation, bit decimation, tilt EQ and reverb." },
        { "saturate", "Saturate: tape-style soft saturation, level-matched." },
        { "decimate", "Decimate: sample-rate and bit reduction." },
        { "tiltEq", "Tilt EQ: one control from dark to bright around 1 kHz." },
        { "verbMix", "Verb Mix: reverb wet amount, equal-power." },
        { "patternEnabled", "Pattern on/off: trigger mode, probability, reverse, pitch quantize and spray." },
        { "triggerMode", "Trigger Mode: Free follows Density; Sync and Euclid lock to the host grid; Audio fires on input hits." },
        { "syncDivision", "Sync Division: note length of one step in Sync and Euclid modes." },
        { "euclidPulses", "Euclid Pulses: hits spread evenly across the steps." },
        { "euclidSteps", "Euclid Steps: length of the cycle." },
        { "euclidRotation", "Euclid Rotation: shifts the pattern start." },
        { "transientSensitivity", "Transient Sensitivity: how easily input hits fire grains (Audio mode)." },
        { "probability", "Probability: chance each trigger actually fires a grain." },
        { "reverseProb", "Reverse Probability: chance a grain plays backwards." },
        { "quantizeScale", "Quantize Scale: snaps grain pitch to this scale. Chromatic = semitones." },
        { "spray", "Spray: random timing and length variation per grain (x0.5-x2)." },
        { "subGrainDepth", "Sub-grain Depth: 0 = grains only, 1 = each grain spawns 2-4 sub-grains, 2 = sub-grains spawn their own." },
        { "stochasticDist", "Timing: how grain onsets are spaced in Free mode." },
        { "feedbackEnabled", "Feedback on/off: re-granulates the plugin's own output." },
        { "feedbackGain", "Feedback Gain: how much output is fed back into the grain source. Soft-clipped, max 95 %." },
        { "feedbackDamp", "Feedback Damp: darkens the feedback loop each pass." },
        { "spectralEnabled", "Spectral on/off: Freeze or Blur the source spectrum." },
        { "spectralMode", "Spectral Mode: Freeze = sustained texture, Blur = phase smear by Blur amount." },
        { "spectralBlurAmount", "Spectral Blur: phase smear of the source spectrum. 0 % = dry." },
        { "envelopeShape", "Envelope Shape: grain amplitude window." },
    };
    if (auto it = hints.find(id); it != hints.end())
        return it->second;

    if (id.startsWith("lfo") && id.length() > 4)
    {
        const auto n = id.substring(3, 4);
        const auto rest = id.substring(4);
        if (rest == "Rate")     return "LFO " + n + " Rate: 0.01 Hz to 8 kHz (audio rate).";
        if (rest == "Depth")    return "LFO " + n + " Depth: overall output scale before the matrix.";
        if (rest == "Phase")    return "LFO " + n + " Phase: start offset in degrees.";
        if (rest == "Sync")     return "LFO " + n + " Sync: lock rate to host tempo.";
        if (rest == "Waveform") return "LFO " + n + " Waveform.";
    }
    if (const auto [slot, part] = matrixSlot(id); slot > 0)
    {
        const auto m = "Mod " + juce::String(slot);
        if (part == "Source") return m + ": route from LFO 1 or LFO 2. Click the lit key again to unset.";
        if (part == "Dest")   return m + " destination: the parameter this route modulates.";
        return m + " amount: bipolar depth, drag left for negative.";
    }
    if (const int s = weightSlot(id); s >= 0)
        return "Slot " + juce::String(s + 1) + " Weight: how often grains pick this sample. Weights are relative.";
    return fullName(id);
}

juce::String ParamFormat::modDestParamID(int destIndex)
{
    return (destIndex > 0 && destIndex < kModDestIDs.size()) ? kModDestIDs[destIndex] : juce::String();
}

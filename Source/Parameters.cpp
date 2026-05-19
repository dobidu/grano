#include "Parameters.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::grainSize, 1 }, "Grain Size",
        juce::NormalisableRange<float>{ 5.0f, 500.0f, 0.0f, 0.3f }, 100.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::density, 1 }, "Density",
        juce::NormalisableRange<float>{ 1.0f, 100.0f, 0.0f, 0.3f }, 10.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("g/s")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::position, 1 }, "Position",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::positionJitter, 1 }, "Position Jitter",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::pitchShift, 1 }, "Pitch Shift",
        juce::NormalisableRange<float>{ -24.0f, 24.0f }, 0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("st")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::stereoSpread, 1 }, "Stereo Spread",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.5f,
        juce::AudioParameterFloatAttributes{}.withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::masterVolume, 1 }, "Master Volume",
        juce::NormalisableRange<float>{ -60.0f, 6.0f }, 0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamIDs::loop, 1 }, "Loop", false));

    // F4a — MOTION module
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamIDs::motionEnabled, 1 }, "Motion", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::wowDepth, 1 }, "Wow Depth",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::wowRate, 1 }, "Wow Rate",
        juce::NormalisableRange<float>{ 0.1f, 2.0f, 0.0f, 0.5f }, 0.5f,
        juce::AudioParameterFloatAttributes{}.withLabel("Hz")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::flutterDepth, 1 }, "Flutter Depth",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::flutterRate, 1 }, "Flutter Rate",
        juce::NormalisableRange<float>{ 3.0f, 20.0f, 0.0f, 0.5f }, 8.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("Hz")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::driftAmount, 1 }, "Drift",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::crackleLevel, 1 }, "Crackle Level",
        juce::NormalisableRange<float>{ -60.0f, 0.0f }, -60.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::crackleColor, 1 }, "Crackle Color",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.5f));

    // F4b — COLOR module
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamIDs::colorEnabled, 1 }, "Color", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::saturate, 1 }, "Saturate",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::decimate, 1 }, "Decimate",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::tiltEq, 1 }, "Tilt EQ",
        juce::NormalisableRange<float>{ -1.0f, 1.0f }, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::verbMix, 1 }, "Verb Mix",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.0f));

    // F4c — PATTERN module
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamIDs::patternEnabled, 1 }, "Pattern", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParamIDs::triggerMode, 1 }, "Trigger Mode",
        juce::StringArray{ "Free", "Sync", "Euclidean", "Audio" }, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParamIDs::syncDivision, 1 }, "Sync Division",
        juce::StringArray{ "1/4", "1/8", "1/16", "1/32", "1/64",
                           "1/4T", "1/8T", "1/16T",
                           "1/4D", "1/8D" }, 1));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ ParamIDs::euclidPulses, 1 }, "Eucl Pulses", 1, 32, 4));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ ParamIDs::euclidSteps, 1 }, "Eucl Steps", 2, 32, 8));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ ParamIDs::euclidRotation, 1 }, "Eucl Rotation", 0, 31, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::transientSensitivity, 1 }, "Transient Sensitivity",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::probability, 1 }, "Probability",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::reverseProb, 1 }, "Reverse Prob",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParamIDs::quantizeScale, 1 }, "Quantize Scale",
        juce::StringArray{ "Chromatic", "Major", "Minor", "Dorian", "Phrygian",
                           "Lydian", "Mixolydian", "Pent Maj", "Pent Min",
                           "Whole Tone", "Octatonic" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::spray, 1 }, "Spray",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.0f));

    // F5 — LFO 1
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::lfo1Rate, 1 }, "LFO1 Rate",
        juce::NormalisableRange<float>{ 0.01f, 8000.0f, 0.0f, 0.2f }, 1.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("Hz")));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParamIDs::lfo1Waveform, 1 }, "LFO1 Waveform",
        juce::StringArray{ "Sine", "Triangle", "Saw", "Square", "Random", "S&H", "Drawable" }, 0));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamIDs::lfo1Sync, 1 }, "LFO1 Sync", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::lfo1Phase, 1 }, "LFO1 Phase",
        juce::NormalisableRange<float>{ 0.0f, 360.0f }, 0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("deg")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::lfo1Depth, 1 }, "LFO1 Depth",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.5f));

    // F5 — LFO 2
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::lfo2Rate, 1 }, "LFO2 Rate",
        juce::NormalisableRange<float>{ 0.01f, 8000.0f, 0.0f, 0.2f }, 1.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("Hz")));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParamIDs::lfo2Waveform, 1 }, "LFO2 Waveform",
        juce::StringArray{ "Sine", "Triangle", "Saw", "Square", "Random", "S&H", "Drawable" }, 0));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamIDs::lfo2Sync, 1 }, "LFO2 Sync", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::lfo2Phase, 1 }, "LFO2 Phase",
        juce::NormalisableRange<float>{ 0.0f, 360.0f }, 0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("deg")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::lfo2Depth, 1 }, "LFO2 Depth",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.5f));

    // F6b — Sub-grain + Stochastic timing
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ ParamIDs::subGrainDepth, 1 }, "Sub-grain Depth", 0, 2, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParamIDs::stochasticDist, 1 }, "Stochastic Distribution",
        juce::StringArray{ "Uniform", "Gaussian", "Poisson",
                           "Exponential", "Pareto", "1/f" }, 0));

    // F6c — Feedback path
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamIDs::feedbackEnabled, 1 }, "Feedback", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::feedbackGain, 1 }, "Feedback Gain",
        juce::NormalisableRange<float>{ 0.0f, 0.95f }, 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::feedbackDamp, 1 }, "Feedback Damp",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.3f));

    // F6c — Spectral processor
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParamIDs::spectralEnabled, 1 }, "Spectral", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParamIDs::spectralMode, 1 }, "Spectral Mode",
        juce::StringArray{ "Freeze", "Blur" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParamIDs::spectralBlurAmount, 1 }, "Spectral Blur",
        juce::NormalisableRange<float>{ 0.0f, 1.0f }, 0.5f));

    // F5 — ModMatrix slots (8 × 3 = 24 params)
    const juce::StringArray kModSources{ "None", "LFO1", "LFO2" };
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
    for (int i = 1; i <= 8; ++i)
    {
        const juce::String srcID = slotParamID(i, "Source");
        const juce::String dstID = slotParamID(i, "Dest");
        const juce::String amtID = slotParamID(i, "Amount");
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ srcID.toRawUTF8(), 1 },
            "Slot " + juce::String(i) + " Source", kModSources, 0));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ dstID.toRawUTF8(), 1 },
            "Slot " + juce::String(i) + " Dest", kModDests, 0));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ amtID.toRawUTF8(), 1 },
            "Slot " + juce::String(i) + " Amount",
            juce::NormalisableRange<float>{ -1.0f, 1.0f }, 0.0f));
    }

    return layout;
}

juce::String slotParamID(int slotIndex, const char* suffix)
{
    return "slot" + juce::String(juce::jlimit(1, 8, slotIndex)) + suffix;
}

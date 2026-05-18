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

    return layout;
}

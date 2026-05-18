#include "PluginProcessor.h"
#include "PluginEditor.h"

GranoAudioProcessor::GranoAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager_.registerBasicFormats(); // WAV, AIFF, FLAC, OGG, MP3 (if enabled)
    engine_.setSource(&sampleBuffer_);
    engine_.setParamPointers(
        apvts_.getRawParameterValue(ParamIDs::grainSize),
        apvts_.getRawParameterValue(ParamIDs::density),
        apvts_.getRawParameterValue(ParamIDs::position),
        apvts_.getRawParameterValue(ParamIDs::positionJitter),
        apvts_.getRawParameterValue(ParamIDs::pitchShift),
        apvts_.getRawParameterValue(ParamIDs::stereoSpread),
        apvts_.getRawParameterValue(ParamIDs::masterVolume),
        apvts_.getRawParameterValue(ParamIDs::loop));
    engine_.setPitchModSource(&motion_);
    color_.setParamPointers(
        apvts_.getRawParameterValue(ParamIDs::colorEnabled),
        apvts_.getRawParameterValue(ParamIDs::saturate),
        apvts_.getRawParameterValue(ParamIDs::decimate),
        apvts_.getRawParameterValue(ParamIDs::tiltEq),
        apvts_.getRawParameterValue(ParamIDs::verbMix));
    motion_.setParamPointers(
        apvts_.getRawParameterValue(ParamIDs::motionEnabled),
        apvts_.getRawParameterValue(ParamIDs::wowDepth),
        apvts_.getRawParameterValue(ParamIDs::wowRate),
        apvts_.getRawParameterValue(ParamIDs::flutterDepth),
        apvts_.getRawParameterValue(ParamIDs::flutterRate),
        apvts_.getRawParameterValue(ParamIDs::driftAmount),
        apvts_.getRawParameterValue(ParamIDs::crackleLevel),
        apvts_.getRawParameterValue(ParamIDs::crackleColor));
    // Timer requires MessageManager; skip in headless test contexts.
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        startTimerHz(30);
}

GranoAudioProcessor::~GranoAudioProcessor()
{
    if (isTimerRunning())
        stopTimer();
}

void GranoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine_.prepare(sampleRate, samplesPerBlock);
    motion_.prepare(sampleRate);
    color_.prepare(sampleRate, samplesPerBlock);
}

void GranoAudioProcessor::releaseResources()
{
    engine_.reset();
    motion_.reset();
    color_.reset();
}

// processBlock runs on the host audio thread — real-time safe.
void GranoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    engine_.processBlock(buffer);
    motion_.processBlock(buffer);
    color_.processBlock(buffer);
}

juce::AudioProcessorEditor* GranoAudioProcessor::createEditor()
{
    return new GranoAudioProcessorEditor(*this);
}

void GranoAudioProcessor::loadSampleFile(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager_.createReaderFor(file));

    if (reader == nullptr)
    {
        lastLoadError_ = "Cannot read: " + file.getFileName();
        return;
    }

    const auto numFrames = static_cast<int>(reader->lengthInSamples);
    if (numFrames <= 0)
    {
        lastLoadError_ = "File contains no audio.";
        return;
    }

    // 1 channel (mono), plus 2 guard samples for linear-interpolation safety.
    auto buf = std::make_unique<juce::AudioBuffer<float>>(1, numFrames + 2);
    buf->clear();

    // Read left channel only into channel 0 (stereo files: left taken, right ignored).
    reader->read(buf.get(), 0, numFrames, 0, true, false);

    lastLoadError_        = {};
    lastLoadedSampleRate_ = reader->sampleRate;
    lastLoadedNumFrames_  = numFrames;
    sampleBuffer_.setPending(std::move(buf), numFrames);
}

void GranoAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts_.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void GranoAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts_.state.getType()))
            apvts_.replaceState(juce::ValueTree::fromXml(*xml));
}

// Required factory function — JUCE calls this to instantiate the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranoAudioProcessor();
}

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
    engine_.setPatternSource(&pattern_);
    engine_.setModMatrixSource(&modMatrix_);
    lfo1_.setParamPointers(
        apvts_.getRawParameterValue(ParamIDs::lfo1Rate),
        apvts_.getRawParameterValue(ParamIDs::lfo1Waveform),
        apvts_.getRawParameterValue(ParamIDs::lfo1Sync),
        apvts_.getRawParameterValue(ParamIDs::lfo1Phase),
        apvts_.getRawParameterValue(ParamIDs::lfo1Depth));
    lfo2_.setParamPointers(
        apvts_.getRawParameterValue(ParamIDs::lfo2Rate),
        apvts_.getRawParameterValue(ParamIDs::lfo2Waveform),
        apvts_.getRawParameterValue(ParamIDs::lfo2Sync),
        apvts_.getRawParameterValue(ParamIDs::lfo2Phase),
        apvts_.getRawParameterValue(ParamIDs::lfo2Depth));
    modMatrix_.setLfos(&lfo1_, &lfo2_);
    for (int i = 1; i <= 8; ++i)
    {
        modMatrix_.setSlotParams(i - 1,
            apvts_.getRawParameterValue(slotParamID(i, "Source")),
            apvts_.getRawParameterValue(slotParamID(i, "Dest")),
            apvts_.getRawParameterValue(slotParamID(i, "Amount")));
    }
    pattern_.setParamPointers(
        apvts_.getRawParameterValue(ParamIDs::patternEnabled),
        apvts_.getRawParameterValue(ParamIDs::triggerMode),
        apvts_.getRawParameterValue(ParamIDs::syncDivision),
        apvts_.getRawParameterValue(ParamIDs::euclidPulses),
        apvts_.getRawParameterValue(ParamIDs::euclidSteps),
        apvts_.getRawParameterValue(ParamIDs::euclidRotation),
        apvts_.getRawParameterValue(ParamIDs::transientSensitivity),
        apvts_.getRawParameterValue(ParamIDs::probability),
        apvts_.getRawParameterValue(ParamIDs::reverseProb),
        apvts_.getRawParameterValue(ParamIDs::quantizeScale),
        apvts_.getRawParameterValue(ParamIDs::spray));
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
    pattern_.prepare(sampleRate);
    lfo1_.prepare(sampleRate);
    lfo2_.prepare(sampleRate);
    modMatrix_.prepare(sampleRate);
}

void GranoAudioProcessor::releaseResources()
{
    engine_.reset();
    motion_.reset();
    color_.reset();
    pattern_.reset();
    lfo1_.reset();
    lfo2_.reset();
    modMatrix_.reset();
}

// processBlock runs on the host audio thread — real-time safe.
void GranoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Feed Pattern BPM for Sync mode + input buffer for Audio-driven transient detection.
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;

    // ModMatrix advances LFOs and computes mod offsets; must run before engine schedules grains.
    modMatrix_.processBlock(buffer.getNumSamples(), bpm);

    pattern_.processBlock(buffer, bpm);
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
    juce::ValueTree combined("GranoCombinedState");
    combined.addChild(apvts_.copyState(), -1, nullptr);
    combined.addChild(snapshots_.serialise(), -1, nullptr);
    if (auto xml = combined.createXml())
        copyXmlToBinary(*xml, destData);
}

void GranoAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        if (tree.hasType("GranoCombinedState"))
        {
            if (auto apvtsChild = tree.getChildWithName(apvts_.state.getType()); apvtsChild.isValid())
                apvts_.replaceState(apvtsChild);
            if (auto snapChild = tree.getChildWithName("Snapshots"); snapChild.isValid())
                snapshots_.deserialise(snapChild);
        }
        else if (tree.hasType(apvts_.state.getType()))
        {
            apvts_.replaceState(tree); // old format — no snapshots
        }
    }
}

// Required factory function — JUCE calls this to instantiate the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranoAudioProcessor();
}

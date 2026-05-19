#include "PluginProcessor.h"
#include "PluginEditor.h"

GranoAudioProcessor::GranoAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager_.registerBasicFormats(); // WAV, AIFF, FLAC, OGG, MP3 (if enabled)
    engine_.setBank(&sampleBank_);
    engine_.setBankParamPointers(
        apvts_.getRawParameterValue(ParamIDs::slot0Weight),
        apvts_.getRawParameterValue(ParamIDs::slot1Weight),
        apvts_.getRawParameterValue(ParamIDs::slot2Weight),
        apvts_.getRawParameterValue(ParamIDs::slot3Weight));
    engine_.setParamPointers(
        apvts_.getRawParameterValue(ParamIDs::grainSize),
        apvts_.getRawParameterValue(ParamIDs::density),
        apvts_.getRawParameterValue(ParamIDs::position),
        apvts_.getRawParameterValue(ParamIDs::positionJitter),
        apvts_.getRawParameterValue(ParamIDs::pitchShift),
        apvts_.getRawParameterValue(ParamIDs::stereoSpread),
        apvts_.getRawParameterValue(ParamIDs::masterVolume),
        apvts_.getRawParameterValue(ParamIDs::loop));
    engine_.setAdvancedParamPointers(
        apvts_.getRawParameterValue(ParamIDs::subGrainDepth),
        apvts_.getRawParameterValue(ParamIDs::stochasticDist));
    engine_.setEnvelopeShapeParam(
        apvts_.getRawParameterValue(ParamIDs::envelopeShape));
    engine_.setFeedbackSource(&feedbackPath_);
    engine_.setFeedbackParamPointers(
        apvts_.getRawParameterValue(ParamIDs::feedbackEnabled),
        apvts_.getRawParameterValue(ParamIDs::feedbackGain),
        apvts_.getRawParameterValue(ParamIDs::feedbackDamp));
    engine_.setSpectralSource(&spectralProcessor_);
    engine_.setSpectralParamPointers(
        apvts_.getRawParameterValue(ParamIDs::spectralEnabled),
        apvts_.getRawParameterValue(ParamIDs::spectralMode),
        apvts_.getRawParameterValue(ParamIDs::spectralBlurAmount));
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
    const bool spectralOn = *apvts_.getRawParameterValue(ParamIDs::spectralEnabled) > 0.5f;
    setLatencySamples(spectralOn ? SpectralProcessor::kFFTSize : 0);
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

    // Feed audio output back into FeedbackPath for re-injection as a grain source.
    {
        const float gain = *apvts_.getRawParameterValue(ParamIDs::feedbackGain);
        const float damp = *apvts_.getRawParameterValue(ParamIDs::feedbackDamp);
        const float* L = buffer.getReadPointer(0);
        const float* R = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr;
        feedbackPath_.process(L, R, buffer.getNumSamples(), gain, damp);
    }
}

juce::AudioProcessorEditor* GranoAudioProcessor::createEditor()
{
    return new GranoAudioProcessorEditor(*this);
}

void GranoAudioProcessor::loadSampleFile(const juce::File& file, int slot)
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

    if (reader->numChannels >= 2)
    {
        // Proper stereo-to-mono downmix: average L and R channels.
        juce::AudioBuffer<float> stereo(2, numFrames + 2);
        stereo.clear();
        reader->read(&stereo, 0, numFrames, 0, true, true);
        auto*       dst = buf->getWritePointer(0);
        const auto* L   = stereo.getReadPointer(0);
        const auto* R   = stereo.getReadPointer(1);
        for (int i = 0; i < numFrames; ++i)
            dst[i] = (L[i] + R[i]) * 0.5f;
    }
    else
    {
        reader->read(buf.get(), 0, numFrames, 0, true, false);
    }

    lastLoadError_        = {};
    lastLoadedSampleRate_ = reader->sampleRate;
    lastLoadedNumFrames_  = numFrames;
    sampleBank_.loadSlot(slot, std::move(buf), numFrames);
    triggerSpectralProcessIfEnabled();
}

void GranoAudioProcessor::triggerSpectralProcessIfEnabled()
{
    const bool on = *apvts_.getRawParameterValue(ParamIDs::spectralEnabled) > 0.5f;
    if (!on || sampleBank_.getReadPointer(0) == nullptr)
        return;
    const int   mode = (int)*apvts_.getRawParameterValue(ParamIDs::spectralMode);
    const float blur = *apvts_.getRawParameterValue(ParamIDs::spectralBlurAmount);
    spectralProcessor_.processSource(
        sampleBank_.getReadPointer(0),
        sampleBank_.getNumSamples(0),
        static_cast<SpectralProcessor::Mode>(juce::jlimit(0, 1, mode)),
        blur);
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

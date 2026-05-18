#include "PluginProcessor.h"
#include "PluginEditor.h"

GranoAudioProcessor::GranoAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager_.registerBasicFormats(); // WAV, AIFF, FLAC, OGG, MP3 (if enabled)
    engine_.setSource(&sampleBuffer_);
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
}

void GranoAudioProcessor::releaseResources()
{
    engine_.reset();
}

// processBlock runs on the host audio thread — real-time safe.
void GranoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    engine_.processBlock(buffer);
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

// Required factory function — JUCE calls this to instantiate the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranoAudioProcessor();
}

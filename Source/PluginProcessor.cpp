#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utils/DebugLog.h"

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
    pFeedbackEnabled_ = apvts_.getRawParameterValue(ParamIDs::feedbackEnabled);
    pFeedbackGain_    = apvts_.getRawParameterValue(ParamIDs::feedbackGain);
    pFeedbackDamp_    = apvts_.getRawParameterValue(ParamIDs::feedbackDamp);
    engine_.setFeedbackParamPointers(pFeedbackEnabled_, pFeedbackGain_, pFeedbackDamp_);
    engine_.setNoSourceModePointer(apvts_.getRawParameterValue(ParamIDs::noSourceMode));
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
    // Spectral params take effect at runtime (UAT-006): flag-and-service via
    // the 30 Hz timer so the FFT never runs on the audio thread.
    apvts_.addParameterListener(ParamIDs::spectralEnabled, this);
    apvts_.addParameterListener(ParamIDs::spectralMode, this);
    apvts_.addParameterListener(ParamIDs::spectralBlurAmount, this);

    // Timer requires MessageManager; skip in headless test contexts.
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        startTimerHz(30);
    GranoLog::write(logId() + " processor created");
}

juce::String GranoAudioProcessor::logId() const
{
    return GranoLog::id(this, 'P');
}

juce::String GranoAudioProcessor::describeSlots() const
{
    juce::String s;
    for (int i = 0; i < MultiSampleBank::kNumSlots; ++i)
    {
        const auto& f = slotFiles_[(size_t)i];
        s << " [" << (i + 1) << ":" << (f.getFullPathName().isEmpty() ? juce::String("-") : f.getFileName())
          << " info=" << slotInfo_[(size_t)i].numFrames << " bank=" << sampleBank_.getNumSamples(i) << "]";
    }
    return s;
}

void GranoAudioProcessor::checkSlotConsistency()
{
    // Diagnostics: the strip shows slotInfo_, the engine plays the bank. A
    // slot whose bank holds audio while its info is empty (or the reverse)
    // for ~1 s is the "name gone, still playing" state — log each change.
    for (int i = 0; i < MultiSampleBank::kNumSlots; ++i)
    {
        const bool info = slotInfo_[(size_t)i].numFrames > 0;
        const bool bank = sampleBank_.getNumSamples(i) > 0;
        auto& n = mismatchTicks_[(size_t)i];
        n = (info != bank) ? n + 1 : 0;
        if (n == 30)
            GranoLog::write(logId() + " MISMATCH slot " + juce::String(i + 1)
                            + (bank ? ": bank plays audio but slot info is empty"
                                    : ": slot info set but bank is empty (audio thread not swapping?)")
                            + " |" + describeSlots());
    }
}

GranoAudioProcessor::~GranoAudioProcessor()
{
    GranoLog::write(logId() + " processor destroyed");
    apvts_.removeParameterListener(ParamIDs::spectralEnabled, this);
    apvts_.removeParameterListener(ParamIDs::spectralMode, this);
    apvts_.removeParameterListener(ParamIDs::spectralBlurAmount, this);
    if (isTimerRunning())
        stopTimer();
}

void GranoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    GranoLog::write(logId() + " prepareToPlay " + juce::String(sampleRate) + " Hz, " + juce::String(samplesPerBlock));
    loadMeasurer_.reset(sampleRate, samplesPerBlock);
    engine_.prepare(sampleRate, samplesPerBlock);
    feedbackPath_.prepare(sampleRate); // clear stale ring, size to session rate (R-25)
    feedbackWasOn_ = false;
    motion_.prepare(sampleRate);
    color_.prepare(sampleRate, samplesPerBlock);
    pattern_.prepare(sampleRate);
    lfo1_.prepare(sampleRate);
    lfo2_.prepare(sampleRate);
    modMatrix_.prepare(sampleRate);
    setLatencySamples(0); // spectral is offline on the source — no audio-path delay (R-10)
}

void GranoAudioProcessor::releaseResources()
{
    GranoLog::write(logId() + " releaseResources");
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
    juce::AudioProcessLoadMeasurer::ScopedTimer loadTimer(loadMeasurer_, buffer.getNumSamples());

    // Host transport.
    double bpm = 120.0;
    bool   playing = false, ppqValid = false;
    double ppq = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())
                bpm = *b;
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition())
            {
                ppq = *q;
                ppqValid = true;
            }
        }
    if (bpm <= 0.0)
        bpm = 120.0;

    // Pattern consumes the host INPUT (transient detection) — before the
    // engine clears the buffer.
    pattern_.processBlock(buffer, bpm);

    // Modulation, grain scheduling and Motion run in short sub-blocks so LFO,
    // wow/flutter/drift values are sampled near each grain's spawn time
    // instead of once per host block (R-24/R-28). Sub-buffers reference the
    // host buffer — no allocation.
    constexpr int kSubBlock = 32;
    const int N     = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 48000.0;
    const double samplesPerBeat = sr * 60.0 / bpm;
    for (int start = 0; start < N; start += kSubBlock)
    {
        const int n = std::min(kSubBlock, N - start);
        juce::AudioBuffer<float> sub(buffer.getArrayOfWritePointers(), numCh, start, n);

        modMatrix_.processBlock(n, bpm);
        engine_.setTransport(bpm, playing, ppqValid,
                             ppq + (double)start / samplesPerBeat);
        engine_.processBlock(sub);
        motion_.processBlock(sub);
    }
    color_.processBlock(buffer);

    // Feed audio output back into FeedbackPath for re-injection as a grain
    // source. Tapped BEFORE master volume so the loop gain doesn't depend on
    // the output level (R-11). Skipped entirely when feedback is off (README:
    // zero CPU cost); ring is cleared on re-enable so stale audio doesn't
    // burst back in. Cached atomic pointers — no string-keyed APVTS lookups.
    {
        const bool fbOn = pFeedbackEnabled_ != nullptr &&
            pFeedbackEnabled_->load(std::memory_order_relaxed) > 0.5f;
        if (fbOn)
        {
            if (!feedbackWasOn_)
                feedbackPath_.reset();
            const float gain = pFeedbackGain_ ? pFeedbackGain_->load(std::memory_order_relaxed) : 0.0f;
            const float damp = pFeedbackDamp_ ? pFeedbackDamp_->load(std::memory_order_relaxed) : 0.0f;
            const float* L = buffer.getReadPointer(0);
            const float* R = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr;
            feedbackPath_.process(L, R, buffer.getNumSamples(), gain, damp);
        }
        feedbackWasOn_ = fbOn;
    }

    // Master volume last, ramped per block (R-14).
    engine_.applyMasterGain(buffer);

    // Meter telemetry: hold the max until the editor consumes it.
    for (int ch = 0; ch < std::min(2, buffer.getNumChannels()); ++ch)
    {
        const float m = buffer.getMagnitude(ch, 0, buffer.getNumSamples());
        auto& peak = outputPeak_[(size_t)ch];
        if (m > peak.load(std::memory_order_relaxed))
            peak.store(m, std::memory_order_relaxed);
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
        GranoLog::write(logId() + " load slot " + juce::String(slot + 1) + " FAILED (cannot read): " + file.getFullPathName());
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
    if (slot >= 0 && slot < MultiSampleBank::kNumSlots)
    {
        slotFiles_[static_cast<size_t>(slot)] = file;
        slotInfo_[static_cast<size_t>(slot)] =
            { file, reader->sampleRate, numFrames, (int)reader->numChannels };
    }
    slotStateGen_.fetch_add(1, std::memory_order_acq_rel);
    const auto gen = sampleBank_.loadSlot(slot, std::move(buf), numFrames);
    if (slot == 0)
    {
        // The new buffer is only PENDING until the audio thread swaps it in;
        // processing now would read the previous sample (R-09). Mark dirty
        // and let the timer retry until slot 0 is live with this generation.
        spectralWaitGen_ = gen;
        spectralDirty_.store(true, std::memory_order_release);
    }
    GranoLog::write(logId() + " load slot " + juce::String(slot + 1) + " gen " + juce::String(gen) + ": "
                    + file.getFullPathName() + (restoringState_ ? " (state restore)" : ""));
    notifyNonParameterStateChanged();
}

void GranoAudioProcessor::notifyNonParameterStateChanged()
{
    if (!restoringState_)
        updateHostDisplay(ChangeDetails().withNonParameterStateChanged(true));
}

void GranoAudioProcessor::unloadSlot(int slot)
{
    if (slot < 0 || slot >= MultiSampleBank::kNumSlots)
        return;
    const bool hadSample = slotFiles_[(size_t)slot].getFullPathName().isNotEmpty()
                           || sampleBank_.getNumSamples(slot) > 0;
    if (hadSample)
        GranoLog::write(logId() + " unload slot " + juce::String(slot + 1)
                        + (restoringState_ ? " (state restore: not in restored state)" : ""));
    slotFiles_[(size_t)slot] = juce::File();
    slotInfo_[(size_t)slot]  = {};
    slotStateGen_.fetch_add(1, std::memory_order_acq_rel);
    if (hadSample)
    {
        sampleBank_.loadSlot(slot, nullptr, 0); // empty record: slot goes silent on swap
        if (slot == 0)
            spectralProcessor_.clear();
    }
    if (hadSample)
        notifyNonParameterStateChanged();
}

void GranoAudioProcessor::applyPendingSpectralUpdate()
{
    if (!spectralDirty_.exchange(false, std::memory_order_acq_rel))
        return;

    const bool on = *apvts_.getRawParameterValue(ParamIDs::spectralEnabled) > 0.5f;

    // A freshly loaded slot stays pending until the audio thread's trySwapAll;
    // until slot 0 holds the generation we're waiting for, stay dirty and
    // retry on the next tick.
    if (on && (sampleBank_.getReadPointer(0) == nullptr ||
               (spectralWaitGen_ != 0 &&
                sampleBank_.getCurrentGeneration(0) != spectralWaitGen_)))
    {
        spectralDirty_.store(true, std::memory_order_release);
        return;
    }
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

    juce::ValueTree paths("SamplePaths");
    for (int slot = 0; slot < MultiSampleBank::kNumSlots; ++slot)
    {
        const auto& f = slotFiles_[static_cast<size_t>(slot)];
        if (f.getFullPathName().isNotEmpty())
        {
            juce::ValueTree entry("Slot");
            entry.setProperty("index", slot, nullptr);
            entry.setProperty("path", f.getFullPathName(), nullptr);
            paths.addChild(entry, -1, nullptr);
        }
    }
    combined.addChild(paths, -1, nullptr);

    if (auto xml = combined.createXml())
        copyXmlToBinary(*xml, destData);
    GranoLog::write(logId() + " getState " + juce::String((int)destData.getSize()) + " bytes"
                    + (juce::MessageManager::existsAndIsCurrentThread() ? "" : " (NOT message thread)")
                    + " |" + describeSlots());
}

void GranoAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    GranoLog::write(logId() + " setState " + juce::String(sizeInBytes) + " bytes"
                    + (juce::MessageManager::existsAndIsCurrentThread() ? "" : " (NOT message thread)")
                    + " | before:" + describeSlots());
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        {
            juce::String paths;
            for (const auto& e : tree.getChildWithName("SamplePaths"))
                paths << " [" << ((int)e.getProperty("index", -1) + 1) << ":"
                      << e.getProperty("path", juce::String()).toString() << "]";
            GranoLog::write(logId() + " setState type " + tree.getType().toString()
                            + " paths:" + (paths.isEmpty() ? juce::String(" none") : paths));
        }
        const juce::ScopedValueSetter<bool> restoring(restoringState_, true);
        if (tree.hasType("GranoCombinedState"))
        {
            if (auto apvtsChild = tree.getChildWithName(apvts_.state.getType()); apvtsChild.isValid())
                apvts_.replaceState(apvtsChild);
            // Restoring a state REPLACES the instance — snapshots and samples
            // absent from it must not survive from the previous one (R-23).
            snapshots_.clear();
            if (auto snapChild = tree.getChildWithName("Snapshots"); snapChild.isValid())
                snapshots_.deserialise(snapChild);

            std::array<bool, MultiSampleBank::kNumSlots> restored{};

            // Reload sample files so the project restores audibly, not just
            // parameter-wise. Missing files surface via the error-label flow
            // without aborting the rest of the restore.
            if (auto pathsChild = tree.getChildWithName("SamplePaths"); pathsChild.isValid())
            {
                for (const auto& entry : pathsChild)
                {
                    const int  slot = entry.getProperty("index", -1);
                    const auto path = entry.getProperty("path", juce::String()).toString();
                    if (slot < 0 || slot >= MultiSampleBank::kNumSlots || path.isEmpty())
                        continue;
                    const juce::File f(path);
                    if (slotFiles_[(size_t)slot] == f && slotInfo_[(size_t)slot].numFrames > 0)
                    {
                        // Same file already loaded (host re-sent the current
                        // state): keep it, no reload or audible restart.
                        restored[(size_t)slot] = true;
                        GranoLog::write(logId() + " setState slot " + juce::String(slot + 1) + " kept (same file)");
                    }
                    else if (f.existsAsFile())
                    {
                        loadSampleFile(f, slot);
                        restored[(size_t)slot] = slotFiles_[(size_t)slot] == f;
                    }
                    else
                    {
                        lastLoadError_ = "Sample not found: " + f.getFileName();
                        GranoLog::write(logId() + " setState slot " + juce::String(slot + 1) + " MISSING file: " + path);
                    }
                }
            }
            for (int slot = 0; slot < MultiSampleBank::kNumSlots; ++slot)
                if (!restored[(size_t)slot])
                    unloadSlot(slot);
        }
        else if (tree.hasType(apvts_.state.getType()))
        {
            apvts_.replaceState(tree); // old format — no snapshots
        }

        // Spectral source + latency must reflect restored spectralEnabled/Mode/
        // Blur even when no sample reload happened (legacy state, samples
        // already loaded). setStateInformation runs on the message thread.
        spectralDirty_.store(true, std::memory_order_release);
        applyPendingSpectralUpdate();
    }
    else
        GranoLog::write(logId() + " setState: not a Grano state (ignored)");
    GranoLog::write(logId() + " setState done | after:" + describeSlots());
}

// Required factory function — JUCE calls this to instantiate the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranoAudioProcessor();
}

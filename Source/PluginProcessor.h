#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Engine/GranularEngine.h"
#include "Engine/MultiSampleBank.h"
#include "Engine/FeedbackPath.h"
#include "Engine/SpectralProcessor.h"
#include "Modules/Motion.h"
#include "Modules/Color.h"
#include "Modules/Pattern.h"
#include "Modulation/Lfo.h"
#include "Modulation/ModMatrix.h"
#include "Modulation/Snapshots.h"
#include "Parameters.h"

// GranoAudioProcessor is the AudioProcessor entry point for the Grano plugin.
//
// Responsibilities (as they accumulate across phases):
//   F0: Empty scaffold — processBlock clears the output buffer.
//   F1: Owns GranularEngine and GrainPool; drives audio generation.
//   F2: Manages SampleBuffer with atomic swap for drag-and-drop loading.
//   F3: Owns the APVTS; all parameters defined in Parameters.cpp.
//   F4-F6: Owns processing module chain (Motion, Color, Pattern, Modulation).
//
// Threading model: see ARCHITECTURE.md §"Threading model".
// processBlock runs on the host audio thread — it must remain real-time safe.
// Never allocate, lock, throw, or make syscalls inside processBlock.

class GranoAudioProcessor : public juce::AudioProcessor,
                             private juce::Timer,
                             private juce::AudioProcessorValueTreeState::Listener
{
public:
    GranoAudioProcessor();
    ~GranoAudioProcessor() override;

    // ── AudioProcessor lifecycle ──────────────────────────────────────────────
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // v1.0 is stereo-only (CLAUDE.md); rejecting mono avoids the aliased
    // L/R double-processing paths in the engine and COLOR module.
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
            && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
    }

    // ── Editor ────────────────────────────────────────────────────────────────
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // ── Identity ──────────────────────────────────────────────────────────────
    const juce::String getName() const override { return "Grano"; }

    // ── MIDI ──────────────────────────────────────────────────────────────────
    // MIDI input is accepted (needed for MIDI learn in F5) but not yet processed.
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    // COLOR verb tail + the 4 s feedback ring keep producing audio after
    // input stops; 0.0 here truncated offline bounces.
    double getTailLengthSeconds() const override { return 4.0; }

    // ── Programs ──────────────────────────────────────────────────────────────
    // Full preset system via APVTS in F3; stubs satisfy the pure-virtual contract.
    int  getNumPrograms()                                        override { return 1; }
    int  getCurrentProgram()                                     override { return 0; }
    void setCurrentProgram(int)                                  override {}
    const juce::String getProgramName(int)                       override { return "Default"; }
    void changeProgramName(int, const juce::String&)             override {}

    // ── State ─────────────────────────────────────────────────────────────────
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS()    noexcept { return apvts_; }
    Snapshots&                          getSnapshots() noexcept { return snapshots_; }

    // ── Sample loading (message thread) ───────────────────────────────────────
    // Called by the editor's FileDragAndDropTarget handler.
    // On success, clears lastLoadError_. On failure, sets it.
    void loadSampleFile(const juce::File& file, int slot = 0);

    // Message thread: clear a slot (no sample, no saved path).
    void unloadSlot(int slot);

    // Message thread: file currently assigned to a slot (empty if none).
    juce::File getSlotFile(int slot) const
    {
        return (slot >= 0 && slot < MultiSampleBank::kNumSlots)
            ? slotFiles_[(size_t)slot] : juce::File();
    }

    const juce::String& getLastLoadError() const noexcept { return lastLoadError_; }

    // Diagnostics: short id of this instance in the session log.
    juce::String logId() const;

    // Message thread: metadata of the sample loaded into a slot (numFrames 0
    // when empty). Lets a reopened editor rebuild the waveform and slot strips.
    struct SlotInfo
    {
        juce::File file;
        double     sampleRate { 0.0 };
        int        numFrames  { 0 };
        int        numChannels{ 0 };
    };
    SlotInfo getSlotInfo(int slot) const
    {
        return (slot >= 0 && slot < MultiSampleBank::kNumSlots)
            ? slotInfo_[(size_t)slot] : SlotInfo{};
    }

    // Bumped on every slot load/unload (incl. state restore). The editor
    // polls it to refresh slot strips and the waveform.
    juce::uint32 getSlotStateGeneration() const noexcept
    {
        return slotStateGen_.load(std::memory_order_acquire);
    }

    // UI telemetry: peak |sample| per output channel since the last call
    // (written by the audio thread, consumed by the editor meter).
    float consumeOutputPeak(int channel) noexcept
    {
        return outputPeak_[(size_t)juce::jlimit(0, 1, channel)]
                   .exchange(0.0f, std::memory_order_relaxed);
    }

    // UI telemetry: processBlock time as a proportion of the block duration.
    double getCpuLoad() const { return loadMeasurer_.getLoadAsProportion(); }

    SampleBuffer&              getSampleBuffer()  noexcept { return sampleBank_.getSlot(0); }
    MultiSampleBank&           getSampleBank()    noexcept { return sampleBank_; }
    GranularEngine&            getEngine()        noexcept { return engine_; }
    const SpectralProcessor&   getSpectralProcessor() const noexcept { return spectralProcessor_; }
    juce::AudioFormatManager&  getFormatManager() noexcept { return formatManager_; }

    double getLastLoadedSampleRate() const noexcept { return lastLoadedSampleRate_; }
    int    getLastLoadedNumFrames()  const noexcept { return lastLoadedNumFrames_; }

    // Message thread: if a spectral parameter changed since the last call,
    // recompute the spectral source and re-report latency. Serviced by the
    // 30 Hz timer; public so tests (no MessageManager) can drive it directly.
    void applyPendingSpectralUpdate();

private:
    void timerCallback() override
    {
        sampleBank_.processRetiredAll();
        applyPendingSpectralUpdate();
        checkSlotConsistency();
    }

    // May be called from any thread (host automation) — only sets a flag.
    void parameterChanged(const juce::String&, float) override
    {
        spectralDirty_.store(true, std::memory_order_release);
    }

    // Trigger spectral processing from slot 0 of the sample bank.
    // Call from message thread when spectral is enabled or source changes.
    void triggerSpectralProcessIfEnabled();

    MultiSampleBank            sampleBank_;    // must be declared before engine_
    FeedbackPath               feedbackPath_;
    SpectralProcessor          spectralProcessor_;
    GranularEngine             engine_;
    Motion                     motion_;        // must be declared after engine_
    Color                      color_;         // driven after motion_ in processBlock
    Pattern                    pattern_;       // consulted by engine scheduler; processed before engine
    Lfo                        lfo1_, lfo2_;   // must be declared before modMatrix_
    ModMatrix                  modMatrix_;
    Snapshots                  snapshots_;
    juce::AudioFormatManager   formatManager_;
    juce::AudioProcessorValueTreeState apvts_{
        *this, nullptr, "GranoState", createParameterLayout()};
    juce::String               lastLoadError_;
    double                     lastLoadedSampleRate_{ 0.0 };
    int                        lastLoadedNumFrames_ { 0 };

    // Source file per bank slot — message thread only. Serialized in
    // getStateInformation so samples reload with the DAW project.
    std::array<juce::File, MultiSampleBank::kNumSlots> slotFiles_{};
    std::array<SlotInfo,   MultiSampleBank::kNumSlots> slotInfo_{};
    std::atomic<juce::uint32> slotStateGen_{ 0 };
    // True while setStateInformation runs: the host is setting the state, so
    // slot loads/unloads must not report it back as a change.
    bool restoringState_{ false };
    // Message thread: tell the host the non-parameter state (sample paths)
    // changed, so it re-reads getStateInformation instead of keeping a stale
    // copy it may restore later (which would empty the slot).
    void notifyNonParameterStateChanged();

    // Diagnostics (Utils/DebugLog): instance id, slot summary, and a 30 Hz
    // check that slot info and the bank agree.
    juce::String describeSlots() const;
    void checkSlotConsistency();
    std::array<int, MultiSampleBank::kNumSlots> mismatchTicks_{};

    // Editor telemetry (not parameters): output peaks, block load.
    std::array<std::atomic<float>, 2> outputPeak_{};
    juce::AudioProcessLoadMeasurer    loadMeasurer_;

    // Set by parameterChanged (any thread), consumed on the message thread.
    std::atomic<bool> spectralDirty_{ false };
    // Message thread: slot-0 load generation the spectral source must be
    // derived from (0 = none pending).
    juce::uint32      spectralWaitGen_{ 0 };

    // Cached APVTS atomics for the audio-thread feedback tap (hard rule 1:
    // no string-keyed getRawParameterValue in processBlock).
    std::atomic<float>* pFeedbackEnabled_{ nullptr };
    std::atomic<float>* pFeedbackGain_   { nullptr };
    std::atomic<float>* pFeedbackDamp_   { nullptr };
    bool                feedbackWasOn_   { false }; // audio thread only

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranoAudioProcessor)
};

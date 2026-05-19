#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Engine/GranularEngine.h"
#include "Engine/SampleBuffer.h"
#include "Modules/Motion.h"
#include "Modules/Color.h"
#include "Modules/Pattern.h"
#include "Modulation/Lfo.h"
#include "Modulation/ModMatrix.h"
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
                             private juce::Timer
{
public:
    GranoAudioProcessor();
    ~GranoAudioProcessor() override;

    // ── AudioProcessor lifecycle ──────────────────────────────────────────────
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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

    double getTailLengthSeconds() const override { return 0.0; }

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

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts_; }

    // ── Sample loading (message thread) ───────────────────────────────────────
    // Called by the editor's FileDragAndDropTarget handler.
    // On success, clears lastLoadError_. On failure, sets it.
    void loadSampleFile(const juce::File& file);

    const juce::String& getLastLoadError() const noexcept { return lastLoadError_; }

    SampleBuffer&              getSampleBuffer()  noexcept { return sampleBuffer_; }
    GranularEngine&            getEngine()        noexcept { return engine_; }
    juce::AudioFormatManager&  getFormatManager() noexcept { return formatManager_; }

    double getLastLoadedSampleRate() const noexcept { return lastLoadedSampleRate_; }
    int    getLastLoadedNumFrames()  const noexcept { return lastLoadedNumFrames_; }

private:
    void timerCallback() override { sampleBuffer_.processRetired(); }

    SampleBuffer               sampleBuffer_;  // must be declared before engine_
    GranularEngine             engine_;
    Motion                     motion_;        // must be declared after engine_
    Color                      color_;         // driven after motion_ in processBlock
    Pattern                    pattern_;       // consulted by engine scheduler; processed before engine
    Lfo                        lfo1_, lfo2_;   // must be declared before modMatrix_
    ModMatrix                  modMatrix_;
    juce::AudioFormatManager   formatManager_;
    juce::AudioProcessorValueTreeState apvts_{
        *this, nullptr, "GranoState", createParameterLayout()};
    juce::String               lastLoadError_;
    double                     lastLoadedSampleRate_{ 0.0 };
    int                        lastLoadedNumFrames_ { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranoAudioProcessor)
};

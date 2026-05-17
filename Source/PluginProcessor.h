#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

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

class GranoAudioProcessor : public juce::AudioProcessor
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
    // APVTS serialisation added in F3. Stubs here keep the plugin valid in F0.
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int)   override {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranoAudioProcessor)
};

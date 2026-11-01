#pragma once
#include "Console.h"
#include "Header.h"
#include "Monitor.h"
#include "SlotStrip.h"
#include "Banks.h"

class GranoAudioProcessor;

// ConsoleView — the whole editor at its 1200 x 780 base size. The editor
// scales it uniformly with an AffineTransform (0.75x - 1.6x).
//
// Owns header, monitor, slot strips and module banks; is the TouchSink that
// drives the command line; shows hover hints in the status strip; refreshes
// telemetry (meter, grains, CPU, slot state, modulation marks) from one timer.

class ConsoleView : public juce::Component,
                    public Console::TouchSink,
                    private juce::Timer
{
public:
    explicit ConsoleView(GranoAudioProcessor&);
    ~ConsoleView() override;

    // Load a file into the slot shown in the monitor (drag-and-drop).
    void loadIntoShownSlot(const juce::File&);
    void setDragHighlight(bool b) { monitor_.setDragHighlight(b); }
    // Run one telemetry tick by hand (headless tests have no message loop).
    void tickTelemetry() { timerCallback(); }

    void paint(juce::Graphics&) override;
    void resized() override;

    // TouchSink
    void controlTouched(juce::Component& from, juce::RangedAudioParameter&, float prevValue) override;
    void controlChanged(juce::RangedAudioParameter&) override;

    // Hover hints
    void mouseMove(const juce::MouseEvent&) override;
    void mouseEnter(const juce::MouseEvent& e) override { mouseMove(e); }
    void mouseExit(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void refreshSlots(bool force);
    void refreshModulation();
    void showCommand(juce::RangedAudioParameter&);
    void message(const juce::String& name, const juce::String& value, const juce::String& sub);
    void loadSlot(int slot, const juce::File&);
    void chooseFile(int slot);
    void showSlot(int slot);
    void setHint(const juce::String&);

    GranoAudioProcessor& proc_;

    CommandLine   cmd_;
    SnapshotBar   snapshots_;
    LoopSection   loop_;
    MasterSection master_;
    Monitor       monitor_;
    std::array<std::unique_ptr<SlotStrip>, 4> slots_;

    Banks::GrainBank    grain_;
    Banks::EngineBank   engine_;
    Banks::EnvelopeBank envelope_;
    Banks::MotionBank   motion_;
    Banks::ColorBank    color_;
    Banks::PatternBank  pattern_;
    Banks::LfoBank      lfo1_, lfo2_;
    Banks::MatrixBank   matrix_;
    std::vector<Console::Bank*> banks_;

    std::map<juce::String, Console::ParamControl*> controls_;
    std::map<juce::String, float> previous_;      // value before the last gesture
    juce::RangedAudioParameter* current_ { nullptr };
    float gestureStart_ { 0.0f };
    Console::Bank* touchedBank_ { nullptr };

    std::unique_ptr<juce::FileChooser> chooser_;
    std::array<juce::String, 4> slotErrors_;
    int shownSlot_ { 0 };
    juce::uint32 slotGen_ { 0xffffffffu };
    int tickCount_ { 0 };

    juce::String hint_, statusLeft_;
    int grains_ { 0 }, cpu_ { 0 }, loaded_ { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleView)
};

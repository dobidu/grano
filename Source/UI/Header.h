#pragma once
#include "Console.h"
#include "../Modulation/Snapshots.h"

// Header strip: command line, snapshot keys, loop key, master section.

// The command line spells out whatever was last touched: full name, the
// previous and default values, and the current value large.
class CommandLine : public juce::Component
{
public:
    CommandLine();
    void show(const juce::String& name, const juce::String& sub, const juce::String& value);
    void paint(juce::Graphics&) override;
private:
    juce::String name_, sub_, value_;
};

// Snapshot keys A-D. Click recalls (or stores into an empty slot);
// Ctrl/Cmd-click stores; right-click opens Store / Recall / Clear.
class SnapshotBar : public juce::Component
{
public:
    SnapshotBar(Snapshots&, juce::AudioProcessorValueTreeState&);
    ~SnapshotBar() override;
    std::function<void(int slot, const juce::String& what)> onAction;
    void refresh();
    void resized() override;
    void paint(juce::Graphics&) override;

private:
    class SnapKey;
    void store(int);
    void recall(int);
    void clear(int);
    void showMenu(int);

    Snapshots& snaps_;
    juce::AudioProcessorValueTreeState& apvts_;
    std::array<std::unique_ptr<SnapKey>, Snapshots::kNumSlots> keys_;
};

// Loop key with status line underneath.
class LoopSection : public juce::Component
{
public:
    explicit LoopSection(juce::RangedAudioParameter& loop);
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    Console::ToggleKey key_;
};

// Master: volume readout, clip LED, stereo segmented meter, volume fader.
class MasterSection : public juce::Component
{
public:
    explicit MasterSection(juce::RangedAudioParameter& volume);
    // 30 Hz: linear peaks since the last tick.
    void pushPeaks(float left, float right);
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    void paintMeter(juce::Graphics&, juce::Rectangle<float>, float level) const;
    Console::HFader fader_;
    juce::RangedAudioParameter& volume_;
    std::array<float, 2> level_ { 0.0f, 0.0f };
    int clipHold_ { 0 };
};

#pragma once
#include "Console.h"

// One sample-bank slot: select key (shows it in the monitor), filename strip
// (click to load), weight fader, clear key. States: empty / loaded / error.
class SlotStrip : public juce::Component
{
public:
    SlotStrip(int index, juce::RangedAudioParameter& weight);

    void setFile(const juce::File&);           // empty File = empty slot
    void setError(const juce::String& message); // empty = no error
    void setShown(bool);
    bool isLoaded() const noexcept { return file_ != juce::File(); }

    std::function<void()> onShow, onLoad, onClear;

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    class NameStrip : public juce::Button
    {
    public:
        explicit NameStrip(SlotStrip& o) : juce::Button("load"), owner_(o) { setMouseClickGrabsKeyboardFocus(false); }
        void paintButton(juce::Graphics&, bool over, bool down) override;
    private:
        SlotStrip& owner_;
    };

    void updateState();

    int index_;
    juce::File file_;
    juce::String error_;
    Console::Key num_;
    NameStrip name_;
    Console::HFader weight_;
    Console::Key clear_;
};

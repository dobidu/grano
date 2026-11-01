#include "SlotStrip.h"

using namespace Theme;

SlotStrip::SlotStrip(int index, juce::RangedAudioParameter& weight)
    : index_(index),
      num_(juce::String(index + 1), C::tape),
      name_(*this),
      weight_(weight, C::mGrain, "Weight"),
      clear_({}, C::tape)
{
    const auto n = juce::String(index + 1);
    setTitle("Slot " + n);
    setFocusContainerType(FocusContainerType::focusContainer);

    num_.setTextSize(15.0f);
    num_.setTitle("Show slot " + n + " in monitor");
    num_.setTooltip("Slot " + n + ": click to show this sample in the monitor.");
    num_.onClick = [this] { if (onShow) onShow(); };

    name_.setTitle("Load sample into slot " + n);
    name_.setTooltip("Slot " + n + ": click to load a sample (or drop a file on the monitor).");
    name_.onClick = [this] { if (onLoad) onLoad(); };

    clear_.setTitle("Clear slot " + n);
    clear_.setTooltip("Clear slot " + n + ".");
    clear_.setIcon([](juce::Graphics& g, juce::Rectangle<float> r, juce::Colour ink)
    {
        const auto c = r.getCentre();
        juce::Path p;
        p.startNewSubPath(c.x - 4.5f, c.y - 4.5f); p.lineTo(c.x + 4.5f, c.y + 4.5f);
        p.startNewSubPath(c.x + 4.5f, c.y - 4.5f); p.lineTo(c.x - 4.5f, c.y + 4.5f);
        g.setColour(ink);
        g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    });
    clear_.onClick = [this] { if (onClear) onClear(); };

    for (juce::Component* c : { (juce::Component*)&num_, (juce::Component*)&name_, (juce::Component*)&weight_, (juce::Component*)&clear_ })
        addAndMakeVisible(c);
    updateState();
}

void SlotStrip::setFile(const juce::File& f)
{
    if (f == file_) return;
    file_ = f;
    if (isLoaded()) error_ = {};
    updateState();
}

void SlotStrip::setError(const juce::String& m)
{
    error_ = m;
    updateState();
}

void SlotStrip::setShown(bool b)
{
    num_.setToggleState(b, juce::dontSendNotification);
}

void SlotStrip::updateState()
{
    clear_.setEnabled(isLoaded());
    weight_.setAlpha(isLoaded() ? 1.0f : 0.4f);
    name_.setDescription(isLoaded() ? file_.getFileName() : error_.isNotEmpty() ? error_ : "empty");
    name_.setTooltip(isLoaded() ? file_.getFullPathName()
                                : error_.isNotEmpty() ? error_
                                : "Slot " + juce::String(index_ + 1) + ": click to load a sample (or drop a file on the monitor).");
    repaint();
    name_.repaint();
}

void SlotStrip::resized()
{
    auto r = getLocalBounds().reduced(6, 0);
    const int cy = getHeight() / 2;
    num_.setBounds(r.removeFromLeft(30).withSizeKeepingCentre(30, 26));
    r.removeFromLeft(6);
    clear_.setBounds(r.removeFromRight(24).withSizeKeepingCentre(24, 26));
    r.removeFromRight(6);
    weight_.setBounds(r.removeFromRight(64).withSizeKeepingCentre(64, 20));
    r.removeFromRight(6);
    name_.setBounds(r.getX(), cy - 13, r.getWidth(), 26);
}

void SlotStrip::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour(C::plate);
    g.fillRoundedRectangle(b, kKeyRadius);
    g.setColour(C::line);
    g.drawRoundedRectangle(b.reduced(0.5f), kKeyRadius, 1.0f);
}

void SlotStrip::NameStrip::paintButton(juce::Graphics& g, bool over, bool)
{
    const auto b = getLocalBounds().toFloat();
    Console::drawStrip(g, b);
    if (over) { g.setColour(C::line2); g.drawRoundedRectangle(b.reduced(0.5f), kStripRadius, 1.0f); }
    const auto f = font(Face::lcd, 14.0f);
    const auto area = b.reduced(7.0f, 0.0f);
    juce::String caption;
    juce::Colour col;
    if (owner_.isLoaded()) { caption = Console::midTruncate(f, owner_.file_.getFileName(), area.getWidth()); col = C::lcd; }
    else if (owner_.error_.isNotEmpty()) { caption = "load failed"; col = C::clip; }
    else { caption = "click to load"; col = C::lcdDim; }
    g.setColour(col);
    g.setFont(f);
    g.drawText(caption, area, juce::Justification::centredLeft, true);
    if (hasKeyboardFocus(false)) Console::drawFocus(g, b);
}

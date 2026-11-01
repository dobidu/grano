#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "GranoTheme.h"

// GranoLAF — LookAndFeel for the stock JUCE widgets the console still uses
// (ComboBox selects, PopupMenu, TooltipWindow, TextButton keys, Label).
// Console controls (faders, encoders, value strips, keys) paint themselves
// from GranoTheme tokens; see Source/UI/Console.h.

class GranoLAF : public juce::LookAndFeel_V4
{
public:
    GranoLAF();

    // Key painting shared by TextButton and the console key components.
    static void paintKey(juce::Graphics&, juce::Rectangle<float> bounds, bool lit,
                         juce::Colour litColour, bool hover, bool down);

    // ── ComboBox (scribble-strip select) ──────────────────────────────────────
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox&, juce::Label&) override;

    // ── PopupMenu ─────────────────────────────────────────────────────────────
    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;
    int getPopupMenuBorderSize() override { return 4; }

    // ── Tooltip ───────────────────────────────────────────────────────────────
    juce::Rectangle<int> getTooltipBounds(const juce::String& tipText, juce::Point<int> screenPos,
                                          juce::Rectangle<int> parentArea) override;
    void drawTooltip(juce::Graphics&, const juce::String& text, int width, int height) override;

    // ── TextButton as console key ─────────────────────────────────────────────
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& bg,
                              bool isMouseOver, bool isButtonDown) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool isMouseOver, bool isButtonDown) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;

    juce::Font getLabelFont(juce::Label&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranoLAF)
};

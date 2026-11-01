#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "LookAndFeel/GranoTheme.h"

// Console — the widget kit of the lighting-desk editor.
//
// Every control binds straight to an APVTS parameter through
// juce::ParameterAttachment (gestures, host automation and undo handled by
// JUCE), paints itself from GranoTheme tokens, exposes an accessible name and
// value, and reports user touches to the nearest TouchSink ancestor so the
// command line can spell out what was touched.

namespace Console
{
    class Bank;

    // ── Touch reporting ───────────────────────────────────────────────────────
    struct TouchSink
    {
        virtual ~TouchSink() = default;
        // Gesture start (prevValue = value before the gesture, real units).
        virtual void controlTouched(juce::Component& from, juce::RangedAudioParameter&, float prevValue) = 0;
        // Value moved by the user during / after a touch.
        virtual void controlChanged(juce::RangedAudioParameter&) = 0;
    };

    void notifyTouched(juce::Component& from, juce::RangedAudioParameter&, float prevValue);
    void notifyChanged(juce::Component& from, juce::RangedAudioParameter&);

    // ── Drawing helpers ───────────────────────────────────────────────────────
    float textWidth(const juce::Font&, const juce::String&);
    juce::String midTruncate(const juce::Font&, const juce::String& text, float maxWidth);
    void drawStrip(juce::Graphics&, juce::Rectangle<float>);
    // Value with the unit set smaller in silkscreen type ("120" + "ms").
    void drawValue(juce::Graphics&, const juce::String& text, juce::Rectangle<float> area,
                   float numberPt, juce::Justification, juce::Colour colour);
    void drawSilk(juce::Graphics&, const juce::String& text, juce::Rectangle<float> area,
                  juce::Justification = juce::Justification::centredLeft,
                  juce::Colour colour = Theme::C::silkDim);
    void drawFocus(juce::Graphics&, juce::Rectangle<float>);

    // ── ParamControl: continuous parameter control base ───────────────────────
    class ParamControl : public juce::Component,
                         public juce::SettableTooltipClient
    {
    public:
        ParamControl(juce::RangedAudioParameter&, juce::Colour edge, juce::String label);
        ~ParamControl() override;

        juce::RangedAudioParameter& getParam() noexcept { return param_; }
        float getValue() const noexcept { return value_; }
        float getNorm() const { return param_.convertTo0to1(value_); }
        juce::String getValueText() const;
        bool isBipolar() const;

        // Matrix modulation depth in normalised units (0 = not modulated).
        void setModDepth(float depth);
        float getModDepth() const noexcept { return modDepth_; }

        void setValueFromUser(float realValue); // complete gesture (accessibility)

        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
        void mouseUp(const juce::MouseEvent&) override;
        void mouseDoubleClick(const juce::MouseEvent&) override;
        void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void mouseEnter(const juce::MouseEvent&) override { repaint(); }
        void mouseExit(const juce::MouseEvent&) override { repaint(); }
        bool keyPressed(const juce::KeyPress&) override;
        void focusGained(FocusChangeType) override { repaint(); }
        void focusLost(FocusChangeType) override { repaint(); }
        std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

    protected:
        enum class Axis { vertical, horizontal };
        void setDrag(Axis axis, float pixelsForFullRange) { axis_ = axis; dragPx_ = pixelsForFullRange; }
        // Track span for click-to-jump: (start, end) along the drag axis in
        // local px, start = minimum. Return {0,0} for no jump (encoders).
        virtual juce::Range<float> jumpSpan() const { return {}; }

        juce::RangedAudioParameter& param_;
        juce::Colour edge_;
        juce::String label_;
        float value_    { 0.0f };
        float modDepth_ { 0.0f };

    private:
        void setNormInGesture(float n);
        float stepNorm() const;

        juce::ParameterAttachment attachment_;
        Axis  axis_ { Axis::vertical };
        float dragPx_ { 140.0f };
        float dragNorm_ { 0.0f };
        float dragStartNorm_ { 0.0f };
        juce::Point<float> dragStart_;
        bool  inGesture_ { false };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamControl)
    };

    // Vertical channel fader: scribble strip on top, track + flat cap below.
    class Fader : public ParamControl
    {
    public:
        using ParamControl::ParamControl;
        void paint(juce::Graphics&) override;
    protected:
        juce::Range<float> jumpSpan() const override;
    private:
        juce::Rectangle<float> track() const;
    };

    // Flat rotary encoder with scribble strip below.
    class Encoder : public ParamControl
    {
    public:
        Encoder(juce::RangedAudioParameter&, juce::Colour edge, juce::String label);
        void paint(juce::Graphics&) override;
    };

    // Compact draggable scribble strip (secondary parameters).
    class ValueStrip : public ParamControl
    {
    public:
        ValueStrip(juce::RangedAudioParameter&, juce::Colour edge, juce::String label);
        void paint(juce::Graphics&) override;
    };

    // Horizontal mini fader (master volume, slot weights).
    class HFader : public ParamControl
    {
    public:
        HFader(juce::RangedAudioParameter&, juce::Colour edge, juce::String label);
        void paint(juce::Graphics&) override;
    protected:
        juce::Range<float> jumpSpan() const override;
    };

    // Bipolar amount bar (matrix): fill grows from the centre.
    class BipolarBar : public ParamControl
    {
    public:
        BipolarBar(juce::RangedAudioParameter&, juce::Colour edge, juce::String label);
        void paint(juce::Graphics&) override;
    protected:
        juce::Range<float> jumpSpan() const override;
    };

    // ── Keys ──────────────────────────────────────────────────────────────────
    // Square lit key. Lit = toggle state.
    class Key : public juce::Button
    {
    public:
        using IconPainter = std::function<void(juce::Graphics&, juce::Rectangle<float>, juce::Colour ink)>;

        Key(const juce::String& caption, juce::Colour litColour);

        void setIcon(IconPainter p) { icon_ = std::move(p); repaint(); }
        void setLitColour(juce::Colour c) { lit_ = c; repaint(); }
        void setUnlitInk(juce::Colour c)  { inkOff_ = c; repaint(); }
        void setTextSize(float pt, Theme::Face face = Theme::Face::silkBold) { pt_ = pt; face_ = face; repaint(); }
        void setIconAbove(bool b) { iconAbove_ = b; }

        void paintButton(juce::Graphics&, bool over, bool down) override;
        void focusGained(FocusChangeType) override { repaint(); }
        void focusLost(FocusChangeType) override { repaint(); }

    private:
        juce::Colour lit_, inkOff_ { Theme::C::silk };
        IconPainter  icon_;
        float        pt_ { 13.0f };
        Theme::Face  face_ { Theme::Face::silkBold };
        bool         iconAbove_ { false };
    };

    // A row / grid of keys selecting one value of a choice / int parameter.
    class ChoiceKeys : public juce::Component
    {
    public:
        // values[i] = parameter value key i selects. clearable: clicking the
        // lit key sets the parameter back to clearValue (matrix sources).
        ChoiceKeys(juce::RangedAudioParameter&, juce::StringArray labels, juce::Colour lit,
                   int columns, std::vector<int> values = {}, bool clearable = false, int clearValue = 0);
        ~ChoiceKeys() override;

        Key& key(int i) { return *keys_[(size_t)i]; }
        int  size() const { return (int)keys_.size(); }
        void setGap(int g) { gap_ = g; resized(); }
        int  getSelected() const { return selected_; }
        std::function<void(int)> onSelect;   // after a user change

        void resized() override;

    private:
        void refresh();
        juce::RangedAudioParameter& param_;
        juce::ParameterAttachment attachment_;
        std::vector<std::unique_ptr<Key>> keys_;
        std::vector<int> values_;
        int  columns_, gap_ { 3 }, clearValue_;
        bool clearable_;
        int  selected_ { -1 };
    };

    // Lit key bound to a bool parameter.
    class ToggleKey : public Key
    {
    public:
        ToggleKey(juce::RangedAudioParameter&, juce::Colour lit,
                  juce::String textOn = "On", juce::String textOff = "Off");
        ~ToggleKey() override;
        std::function<void(bool)> onState;   // any change (user or host)
        void clicked() override;
    private:
        juce::RangedAudioParameter& param_;
        juce::ParameterAttachment attachment_;
        juce::String on_, off_;
    };

    // Scribble-strip select bound to a choice parameter.
    class ChoiceSelect : public juce::ComboBox
    {
    public:
        // labels default to the parameter's choices.
        ChoiceSelect(juce::RangedAudioParameter&, juce::StringArray labels = {}, bool silkType = false);
        ~ChoiceSelect() override;
    private:
        juce::RangedAudioParameter& param_;
        juce::ParameterAttachment attachment_;
        bool updating_ { false };
    };

    // ── Bank: plate with console tape label ───────────────────────────────────
    class Bank : public juce::Component
    {
    public:
        Bank(juce::String title, juce::Colour edge);
        ~Bank() override;

        juce::Component& body() noexcept { return body_; }
        juce::Colour getEdge() const noexcept { return edge_; }

        // Module bypass: on-key in the tape; body dims when off.
        void setOnParam(juce::RangedAudioParameter&);
        void setTouched(bool);

        void paint(juce::Graphics&) override;
        void resized() override;

    private:
        juce::String title_;
        juce::Colour edge_;
        juce::Component body_;
        std::unique_ptr<ToggleKey> onKey_;
        bool touched_ { false }, off_ { false };
    };

    // Sub-panel inside a bank (Feedback, Spectral): title + on-key on the
    // left, controls in panel() dimmed while off.
    class SubPanel : public juce::Component
    {
    public:
        SubPanel(juce::String title, juce::RangedAudioParameter& onParam, juce::Colour edge, bool leftSeam);
        juce::Component& panel() noexcept { return panel_; }
        void paint(juce::Graphics&) override;
        void resized() override;
    private:
        juce::String title_;
        bool leftSeam_;
        ToggleKey onKey_;
        juce::Component panel_;
    };

    // Static silkscreen label.
    class Silk : public juce::Component
    {
    public:
        explicit Silk(juce::String t, juce::Justification j = juce::Justification::centredLeft)
            : text_(std::move(t)), just_(j) { setInterceptsMouseClicks(false, false); }
        void setText(const juce::String& t) { if (t != text_) { text_ = t; repaint(); } }
        void setColour(juce::Colour c) { colour_ = c; repaint(); }
        // Plain: sentence-case info copy, wrapped (Pattern context notes).
        void setPlain(bool p) { plain_ = p; repaint(); }
        void paint(juce::Graphics& g) override
        {
            if (!plain_) { drawSilk(g, text_, getLocalBounds().toFloat(), just_, colour_); return; }
            g.setColour(colour_);
            g.setFont(font(Theme::Face::text, 13.0f));
            g.drawFittedText(text_, getLocalBounds(), juce::Justification::topLeft, 6, 1.0f);
        }
    private:
        bool plain_ { false };
        juce::String text_;
        juce::Justification just_;
        juce::Colour colour_ { Theme::C::silkDim };
    };

    // Recursively collect all ParamControls under a component.
    void collectControls(juce::Component&, std::vector<ParamControl*>&);
}

#include "Banks.h"
#include "ParamFormat.h"
#include "../Parameters.h"
#include "../Engine/EnvelopeShapes.h"
#include "../Modules/Pattern.h"

using namespace Theme;
using namespace Console;

namespace Banks
{
namespace
{
    juce::RangedAudioParameter& P(APVTS& a, const juce::String& id)
    {
        auto* p = a.getParameter(id);
        jassert(p != nullptr);
        return *p;
    }

    // Thin vertical / horizontal seam line.
    class Seam : public juce::Component
    {
    public:
        Seam() { setInterceptsMouseClicks(false, false); }
        void paint(juce::Graphics& g) override { g.fillAll(C::line); }
    };

    // Repaints its owner whenever any listed parameter changes.
    class ParamWatcher
    {
    public:
        ParamWatcher(juce::Component& owner, APVTS& a, std::initializer_list<const char*> ids)
        {
            for (auto* id : ids)
            {
                auto att = std::make_unique<juce::ParameterAttachment>(P(a, id), [&owner](float) { owner.repaint(); }, nullptr);
                atts_.push_back(std::move(att));
            }
        }
    private:
        std::vector<std::unique_ptr<juce::ParameterAttachment>> atts_;
    };
}

juce::Component& OwningBank::seam(juce::Component& parent, juce::Rectangle<int> bounds)
{
    return add<Seam>(parent, bounds);
}

// ── GRAIN ─────────────────────────────────────────────────────────────────────
GrainBank::GrainBank(APVTS& a) : OwningBank("Grain", C::mGrain)
{
    const std::array<std::pair<const char*, const char*>, 6> f{ {
        { ParamIDs::grainSize, "Size" }, { ParamIDs::density, "Density" }, { ParamIDs::position, "Position" },
        { ParamIDs::positionJitter, "Jitter" }, { ParamIDs::pitchShift, "Pitch" }, { ParamIDs::stereoSpread, "Spread" } } };
    for (int i = 0; i < 6; ++i)
        add<Fader>(body(), { 10 + i * 85, 6, 80, 160 }, P(a, f[(size_t)i].first), C::mGrain, f[(size_t)i].second);
}

// ── ENGINE ────────────────────────────────────────────────────────────────────
EngineBank::EngineBank(APVTS& a) : OwningBank("Engine", C::mEngine)
{
    add<Silk>(body(), { 12, 4, 110, 16 }, "Sub-grain");
    auto& sub = add<ChoiceKeys>(body(), { 12, 22, 108, 52 }, P(a, ParamIDs::subGrainDepth),
                                juce::StringArray{ "0", "1", "2" }, C::mEngine, 3);
    const char* subHints[] = { "grains only", "each grain spawns 2-4 sub-grains", "sub-grains spawn their own" };
    for (int i = 0; i < 3; ++i)
    {
        sub.key(i).setTextSize(18.0f);
        sub.key(i).setTooltip("Sub-grain Depth " + juce::String(i) + ": " + subHints[i] + ".");
    }

    add<Silk>(body(), { 132, 4, 110, 16 }, "Timing");
    auto& dist = add<ChoiceKeys>(body(), { 132, 22, 258, 53 }, P(a, ParamIDs::stochasticDist),
                                 juce::StringArray{ "Uniform", "Gauss", "Poisson", "Expon", "Pareto", "1/f" }, C::mEngine, 3);
    const char* distHints[] = { "even random spacing", "clustered around the mean", "bursts with gaps",
                                "memoryless, irregular", "rare long gaps", "correlated, organic drift" };
    for (int i = 0; i < 6; ++i)
        dist.key(i).setTooltip("Timing " + ParamFormat::valueText(ParamIDs::stochasticDist, (float)i) + ": " + distHints[i] + ".");

    auto& fb = add<SubPanel>(body(), { 12, 82, 180, 86 }, "Feedback", P(a, ParamIDs::feedbackEnabled), C::mEngine, false);
    add<Encoder>(fb.panel(), { 0, 0, 50, 80 }, P(a, ParamIDs::feedbackGain), C::mEngine, "Gain");
    add<Encoder>(fb.panel(), { 54, 0, 50, 80 }, P(a, ParamIDs::feedbackDamp), C::mEngine, "Damp");

    auto& sp = add<SubPanel>(body(), { 196, 82, 196, 86 }, "Spectral", P(a, ParamIDs::spectralEnabled), C::mEngine, true);
    auto& mode = add<ChoiceKeys>(sp.panel(), { 0, 2, 54, 55 }, P(a, ParamIDs::spectralMode),
                                 juce::StringArray{ "Freeze", "Blur" }, C::mEngine, 1);
    mode.key(0).setTooltip("Spectral Freeze: sustained texture from the source spectrum.");
    mode.key(1).setTooltip("Spectral Blur: smears source phase by Blur amount.");
    auto& blur = add<Encoder>(sp.panel(), { 58, 0, 54, 80 }, P(a, ParamIDs::spectralBlurAmount), C::mEngine, "Blur");
    modeAttach_ = std::make_unique<juce::ParameterAttachment>(P(a, ParamIDs::spectralMode),
        [&blur](float v) { blur.setAlpha(v > 0.5f ? 1.0f : 0.4f); }, nullptr);
    modeAttach_->sendInitialUpdate();
}

// ── ENVELOPE ──────────────────────────────────────────────────────────────────
namespace
{
    EnvelopeShape shapeAt(int i) { return static_cast<EnvelopeShape>(juce::jlimit(0, 4, i)); }

    juce::Path envelopePath(EnvelopeShape s, juce::Rectangle<float> r, int points)
    {
        juce::Path p;
        for (int i = 0; i <= points; ++i)
        {
            const float x = (float)i / (float)points;
            const float ph = juce::jlimit(0.0005f, 0.9995f, x);
            const auto pt = juce::Point<float>(r.getX() + x * r.getWidth(),
                                               r.getBottom() - applyEnvelope(s, ph) * r.getHeight());
            if (i == 0) p.startNewSubPath(pt); else p.lineTo(pt);
        }
        return p;
    }

    // Large preview of the selected envelope + grain length readout.
    class EnvelopeView : public juce::Component
    {
    public:
        explicit EnvelopeView(APVTS& a)
            : shape_(*a.getRawParameterValue(ParamIDs::envelopeShape)),
              size_(P(a, ParamIDs::grainSize)),
              watch_(*this, a, { ParamIDs::envelopeShape, ParamIDs::grainSize })
        {
            setInterceptsMouseClicks(false, false);
        }
        void paint(juce::Graphics& g) override
        {
            const auto area = juce::Rectangle<float>(0.0f, 0.0f, (float)getWidth(), 36.0f);
            g.setColour(C::well);
            g.fillRect(area);
            const auto r = area.reduced(4.0f, 3.0f);
            auto p = envelopePath(shapeAt(juce::roundToInt(shape_.load())), r, 110);
            auto fill = p;
            fill.lineTo(r.getRight(), r.getBottom());
            fill.lineTo(r.getX(), r.getBottom());
            fill.closeSubPath();
            g.setColour(C::mEngine.withAlpha(0.18f));
            g.fillPath(fill);
            g.setColour(C::mEngine);
            g.strokePath(p, juce::PathStrokeType(1.5f));
            const auto row = juce::Rectangle<float>(0.0f, 40.0f, (float)getWidth(), 16.0f);
            drawSilk(g, "Grain length", row);
            drawValue(g, ParamFormat::valueText(ParamIDs::grainSize, size_.convertFrom0to1(size_.getValue())),
                      row, 14.0f, juce::Justification::centredRight, C::lcd);
        }
    private:
        std::atomic<float>& shape_;
        juce::RangedAudioParameter& size_;
        ParamWatcher watch_;
    };
}

EnvelopeBank::EnvelopeBank(APVTS& a) : OwningBank("Envelope", C::mEngine)
{
    auto& keys = add<ChoiceKeys>(body(), { 12, 8, 200, 108 }, P(a, ParamIDs::envelopeShape),
                                 juce::StringArray{ "Hann", "Tukey", "Gauss", "Triangle", "Rect" }, C::mEngine, 3);
    keys.setGap(4);
    for (int i = 0; i < keys.size(); ++i)
    {
        keys.key(i).setIconAbove(true);
        keys.key(i).setTooltip("Envelope " + ParamFormat::valueText(ParamIDs::envelopeShape, (float)i) + ": grain amplitude shape.");
        keys.key(i).setIcon([i](juce::Graphics& g, juce::Rectangle<float> r, juce::Colour ink)
        {
            const auto box = juce::Rectangle<float>(40.0f, 22.0f).withCentre(r.getCentre()).reduced(2.0f);
            g.setColour(ink);
            g.strokePath(envelopePath(shapeAt(i), box, 40), juce::PathStrokeType(1.6f));
        });
    }
    add<EnvelopeView>(body(), { 12, 118, 200, 56 }, a);
}

// ── MOTION / COLOR ────────────────────────────────────────────────────────────
MotionBank::MotionBank(APVTS& a) : OwningBank("Motion", C::mMotion)
{
    setOnParam(P(a, ParamIDs::motionEnabled));
    const std::array<std::pair<const char*, const char*>, 7> e{ {
        { ParamIDs::wowDepth, "Wow" }, { ParamIDs::wowRate, "Rate" }, { ParamIDs::flutterDepth, "Flutter" },
        { ParamIDs::flutterRate, "Rate" }, { ParamIDs::driftAmount, "Drift" }, { ParamIDs::crackleLevel, "Crackle" },
        { ParamIDs::crackleColor, "Tone" } } };
    for (int i = 0; i < 7; ++i)
    {
        const int x = 8 + i * 57 + (i >= 2 ? 4 : 0) + (i >= 4 ? 4 : 0) + (i >= 5 ? 4 : 0);
        add<Encoder>(body(), { x, 14, 53, 82 }, P(a, e[(size_t)i].first), C::mMotion, e[(size_t)i].second);
    }
    for (int x : { 120, 238, 298 })
        seam(body(), { x, 10, 1, 96 });
}

ColorBank::ColorBank(APVTS& a) : OwningBank("Color", C::mColor)
{
    setOnParam(P(a, ParamIDs::colorEnabled));
    const std::array<std::pair<const char*, const char*>, 4> e{ {
        { ParamIDs::saturate, "Saturate" }, { ParamIDs::decimate, "Decimate" },
        { ParamIDs::tiltEq, "Tilt" }, { ParamIDs::verbMix, "Verb" } } };
    for (int i = 0; i < 4; ++i)
        add<Encoder>(body(), { 8 + i * 61, 14, 59, 82 }, P(a, e[(size_t)i].first), C::mColor, e[(size_t)i].second);
}

// ── PATTERN ───────────────────────────────────────────────────────────────────
namespace
{
    class EuclidRing : public juce::Component
    {
    public:
        explicit EuclidRing(APVTS& a)
            : pulses_(*a.getRawParameterValue(ParamIDs::euclidPulses)),
              steps_ (*a.getRawParameterValue(ParamIDs::euclidSteps)),
              rot_   (*a.getRawParameterValue(ParamIDs::euclidRotation)),
              watch_(*this, a, { ParamIDs::euclidPulses, ParamIDs::euclidSteps, ParamIDs::euclidRotation })
        {
            setInterceptsMouseClicks(false, false);
            setTitle("Euclidean rhythm");
        }
        void paint(juce::Graphics& g) override
        {
            const int n = juce::jlimit(2, 32, juce::roundToInt(steps_.load()));
            const int k = juce::jmin(juce::roundToInt(pulses_.load()), n);
            const auto pat = Pattern::buildEuclidean(k, n, juce::roundToInt(rot_.load()));
            const auto c = getLocalBounds().toFloat().getCentre();
            const float R = (float)getWidth() * 0.5f - 6.0f;
            g.setColour(C::line);
            g.drawEllipse(juce::Rectangle<float>(R * 2.0f, R * 2.0f).withCentre(c), 1.0f);
            const float dot = n > 20 ? 2.6f : 3.6f;
            for (int i = 0; i < n; ++i)
            {
                const float ang = (float)i / (float)n * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::halfPi;
                const auto pt = c + juce::Point<float>(std::cos(ang), std::sin(ang)) * R;
                const auto r = juce::Rectangle<float>(dot * 2.0f, dot * 2.0f).withCentre(pt);
                if (pat[(size_t)i]) { g.setColour(C::mPattern); g.fillEllipse(r); }
                else { g.setColour(C::well); g.fillEllipse(r); g.setColour(C::ringOff); g.drawEllipse(r, 1.0f); }
            }
            g.setColour(C::lcd);
            g.setFont(font(Face::lcd, 13.0f));
            g.drawText(juce::String(k) + "/" + juce::String(n), getLocalBounds(), juce::Justification::centred, false);
        }
    private:
        std::atomic<float>& pulses_; std::atomic<float>& steps_; std::atomic<float>& rot_;
        ParamWatcher watch_;
    };

    class InfoText : public Silk
    {
    public:
        InfoText() : Silk({}) { setPlain(true); }
    };
}

PatternBank::PatternBank(APVTS& a) : OwningBank("Pattern", C::mPattern), apvts_(a)
{
    setOnParam(P(a, ParamIDs::patternEnabled));
    auto& trig = add<ChoiceKeys>(body(), { 10, 6, 217, 28 }, P(a, ParamIDs::triggerMode),
                                 juce::StringArray{ "Free", "Sync", "Euclid", "Audio" }, C::mPattern, 4);
    const char* trigHints[] = { "grains follow Density and Timing", "one grain per host-synced step",
                                "Euclidean rhythm locked to the host grid", "grains fire on input transients" };
    for (int i = 0; i < 4; ++i)
        trig.key(i).setTooltip("Trigger " + ParamFormat::valueText(ParamIDs::triggerMode, (float)i) + ": " + trigHints[i] + ".");

    divLabel_   = &add<Silk>(body(), { 10, 46, 40, 28 }, "Div");
    div_        = &add<ChoiceSelect>(body(), { 52, 46, 96, 28 }, P(a, ParamIDs::syncDivision));
    scaleLabel_ = &add<Silk>(body(), { 10, 84, 40, 28 }, "Scale");
    scale_      = &add<ChoiceSelect>(body(), { 52, 84, 150, 28 }, P(a, ParamIDs::quantizeScale));

    seam(body(), { 234, 4, 1, 112 });
    ring_ = &add<EuclidRing>(body(), { 240, 18, 72, 72 }, a);
    euclid_[0] = &add<ValueStrip>(body(), { 316, 12, 66, 28 }, P(a, ParamIDs::euclidPulses),   C::mPattern, "Puls");
    euclid_[1] = &add<ValueStrip>(body(), { 316, 46, 66, 28 }, P(a, ParamIDs::euclidSteps),    C::mPattern, "Step");
    euclid_[2] = &add<ValueStrip>(body(), { 316, 80, 66, 28 }, P(a, ParamIDs::euclidRotation), C::mPattern, "Rot");
    sens_ = &add<Encoder>(body(), { 244, 8, 62, 82 }, P(a, ParamIDs::transientSensitivity), C::mPattern, "Sens");
    info_ = &add<InfoText>(body(), { 244, 10, 140, 80 });

    add<ValueStrip>(body(), { 392, 12, 84, 28 }, P(a, ParamIDs::probability), C::mPattern, "Prob");
    add<ValueStrip>(body(), { 392, 46, 84, 28 }, P(a, ParamIDs::reverseProb), C::mPattern, "Rev");
    add<ValueStrip>(body(), { 392, 80, 84, 28 }, P(a, ParamIDs::spray),       C::mPattern, "Spray");

    modeAttach_ = std::make_unique<juce::ParameterAttachment>(P(a, ParamIDs::triggerMode), [this](float) { layoutForMode(); }, nullptr);
    divAttach_  = std::make_unique<juce::ParameterAttachment>(P(a, ParamIDs::syncDivision), [this](float) { layoutForMode(); }, nullptr);
    layoutForMode();
}

void PatternBank::layoutForMode()
{
    const int m = juce::roundToInt(apvts_.getRawParameterValue(ParamIDs::triggerMode)->load());
    const bool sync = m == 1, eu = m == 2, audio = m == 3;
    divLabel_->setVisible(sync || eu);
    div_->setVisible(sync || eu);
    const int sy = (sync || eu) ? 84 : 46;
    scaleLabel_->setTopLeftPosition(10, sy);
    scale_->setTopLeftPosition(52, sy);
    ring_->setVisible(eu);
    for (auto* c : euclid_) c->setVisible(eu);
    sens_->setVisible(audio);

    info_->setVisible(!eu);
    info_->setBounds(audio ? 316 : 244, 10, audio ? 70 : 140, 96);
    const auto divText = ParamFormat::valueText(ParamIDs::syncDivision,
                                                apvts_.getRawParameterValue(ParamIDs::syncDivision)->load());
    info_->setText(m == 0 ? "Grains follow Density and the Timing distribution."
                 : sync   ? "One grain per " + divText + " step, locked to host tempo."
                 : audio  ? "Input hits fire grains." : juce::String());
}

// ── LFO ───────────────────────────────────────────────────────────────────────
namespace
{
    float smoothRandom(float x)
    {
        static const float r[] = { 0.2f, -0.7f, 0.9f, -0.1f, 0.5f, -0.9f, 0.3f, 0.7f, -0.4f, 0.1f, -0.6f, 0.8f };
        const float t = x * 6.0f;
        const int i = (int)std::floor(t);
        const float f = t - (float)i, u = f * f * (3.0f - 2.0f * f);
        const float a = r[i % 12], b = r[(i + 1) % 12];
        return a + (b - a) * u;
    }
    float sampleHold(float x)
    {
        static const float r[] = { 0.2f, -0.7f, 0.9f, -0.1f, 0.5f, -0.9f, 0.3f, 0.7f, -0.4f, 0.1f, -0.6f, 0.8f };
        return r[(int)std::floor(x * 6.0f) % 12];
    }
    // Display shape of each waveform over one cycle (x in [0,1)) in [-1, 1].
    float waveAt(int w, float x)
    {
        x = x - std::floor(x);
        switch (w)
        {
            case 0: return std::sin(juce::MathConstants<float>::twoPi * x);
            case 1: return 1.0f - 4.0f * std::abs(std::fmod(x + 0.25f, 1.0f) - 0.5f);
            case 2: return 2.0f * std::fmod(x + 0.5f, 1.0f) - 1.0f;
            case 3: return x < 0.5f ? 1.0f : -1.0f;
            case 4: return smoothRandom(x);
            case 5: return sampleHold(x);
            default: return 0.0f;
        }
    }

    class LfoPreview : public juce::Component
    {
    public:
        LfoPreview(APVTS& a, int n)
            : rate_ (*a.getRawParameterValue("lfo" + juce::String(n) + "Rate")),
              wave_ (*a.getRawParameterValue("lfo" + juce::String(n) + "Waveform")),
              depth_(*a.getRawParameterValue("lfo" + juce::String(n) + "Depth")),
              phase_(*a.getRawParameterValue("lfo" + juce::String(n) + "Phase")),
              watch_(*this, a, { n == 1 ? ParamIDs::lfo1Rate : ParamIDs::lfo2Rate,
                                 n == 1 ? ParamIDs::lfo1Waveform : ParamIDs::lfo2Waveform,
                                 n == 1 ? ParamIDs::lfo1Depth : ParamIDs::lfo2Depth,
                                 n == 1 ? ParamIDs::lfo1Phase : ParamIDs::lfo2Phase })
        {
            setInterceptsMouseClicks(false, false);
        }
        void paint(juce::Graphics& g) override
        {
            const auto b = getLocalBounds().toFloat();
            g.setColour(C::well);
            g.fillRect(b);
            const float mid = b.getCentreY();
            g.setColour(C::lfoAxis);
            g.fillRect(0.0f, std::round(mid), b.getWidth(), 1.0f);
            const int w = juce::roundToInt(wave_.load());
            const float dep = depth_.load(), ph = phase_.load() / 360.0f;
            const float amp = (b.getHeight() * 0.5f - 5.0f) * dep;
            juce::Path p;
            for (int i = 0; i <= (int)b.getWidth(); ++i)
            {
                const float x = (float)i / b.getWidth();
                const auto pt = juce::Point<float>((float)i, mid - waveAt(w, x + ph) * amp);
                if (i == 0) p.startNewSubPath(pt); else p.lineTo(pt);
            }
            g.setColour(C::mMod);
            g.strokePath(p, juce::PathStrokeType(1.5f));
            if (rate_.load() >= 25.0f)
            {
                g.setColour(C::lcd);
                g.setFont(font(Face::silkBold, 13.0f, 0.1f));
                g.drawText("AUDIO RATE", b.reduced(6.0f, 4.0f), juce::Justification::topLeft, false);
            }
        }
    private:
        std::atomic<float>& rate_; std::atomic<float>& wave_; std::atomic<float>& depth_; std::atomic<float>& phase_;
        ParamWatcher watch_;
    };
}

LfoBank::LfoBank(APVTS& a, int n) : OwningBank("LFO " + juce::String(n), C::mMod)
{
    const auto id = [n](const char* s) { return "lfo" + juce::String(n) + s; };
    add<LfoPreview>(body(), { 10, 4, 140, 52 }, a, n);
    auto& waves = add<ChoiceKeys>(body(), { 10, 60, 140, 22 }, P(a, id("Waveform")),
                                  juce::StringArray{ "", "", "", "", "", "" }, C::mMod, 6);
    waves.setGap(2);
    for (int i = 0; i < waves.size(); ++i)
        waves.key(i).setIcon([i](juce::Graphics& g, juce::Rectangle<float> r, juce::Colour ink)
        {
            const auto box = juce::Rectangle<float>(20.0f, 12.0f).withCentre(r.getCentre());
            juce::Path p;
            for (int k = 0; k <= 24; ++k)
            {
                const float x = (float)k / 24.0f;
                const float y = i >= 4 ? waveAt(i, x * 0.5f) : waveAt(i, x);
                const auto pt = juce::Point<float>(box.getX() + x * box.getWidth(), box.getCentreY() - y * box.getHeight() * 0.42f);
                if (k == 0) p.startNewSubPath(pt); else p.lineTo(pt);
            }
            g.setColour(ink);
            g.strokePath(p, juce::PathStrokeType(1.5f));
        });
    auto& sync = add<ToggleKey>(body(), { 10, 86, 140, 24 }, P(a, id("Sync")), C::mMod,
                                juce::String::fromUTF8("Sync to host \xc2\xb7 on"),
                                juce::String::fromUTF8("Sync to host \xc2\xb7 off"));
    sync.setTitle("LFO " + juce::String(n) + " Sync");
    add<Encoder>(body(), { 160, 0, 62, 82 }, P(a, id("Rate")), C::mMod, "Rate");
    add<Encoder>(body(), { 226, 0, 62, 82 }, P(a, id("Depth")), C::mMod, "Depth");
    add<ValueStrip>(body(), { 160, 82, 128, 28 }, P(a, id("Phase")), C::mMod, "Phase");
}

// ── MOD MATRIX ────────────────────────────────────────────────────────────────
class MatrixBank::Row : public juce::Component
{
public:
    Row(APVTS& a, int slot)
        : slot_(slot),
          source_(P(a, slotParamID(slot, "Source")), juce::StringArray{ "L1", "L2" }, C::mMod, 2,
                  std::vector<int>{ 1, 2 }, true, 0),
          dest_(P(a, slotParamID(slot, "Dest")), {}, true),
          amount_(P(a, slotParamID(slot, "Amount")), C::mMod, "Amount"),
          src_(*a.getRawParameterValue(slotParamID(slot, "Source"))),
          dst_(*a.getRawParameterValue(slotParamID(slot, "Dest")))
    {
        setTitle("Mod " + juce::String(slot));
        setFocusContainerType(FocusContainerType::focusContainer);
        source_.setGap(2);
        source_.key(0).setTooltip("Mod " + juce::String(slot) + ": route from LFO 1. Click again to unset.");
        source_.key(1).setTooltip("Mod " + juce::String(slot) + ": route from LFO 2. Click again to unset.");
        addAndMakeVisible(source_);
        addAndMakeVisible(dest_);
        addAndMakeVisible(amount_);
        refresh();
    }
    void refresh()
    {
        const bool dead = src_.load() < 0.5f || dst_.load() < 0.5f;
        dest_.setAlpha(dead ? 0.45f : 1.0f);
        amount_.setAlpha(dead ? 0.45f : 1.0f);
    }
    void resized() override
    {
        const int w = getWidth();
        source_.setBounds(20, 2, 58, 24);
        amount_.setBounds(w - 60, 2, 60, 24);
        dest_.setBounds(84, 2, w - 84 - 66, 24);
    }
    void paint(juce::Graphics& g) override
    {
        g.setColour(C::silkDim);
        g.setFont(font(Face::lcd, 13.0f));
        g.drawText(juce::String(slot_), juce::Rectangle<int>(0, 0, 14, getHeight()), juce::Justification::centredRight, false);
    }
private:
    int slot_;
    ChoiceKeys source_;
    ChoiceSelect dest_;
    BipolarBar amount_;
    std::atomic<float>& src_;
    std::atomic<float>& dst_;
};

MatrixBank::MatrixBank(APVTS& a) : OwningBank("Mod matrix", C::mMod)
{
    constexpr int colW = (552 - 20 - 18) / 2;
    for (int i = 0; i < 8; ++i)
    {
        const int col = i % 2, row = i / 2;
        rows_.push_back(&add<Row>(body(), { 10 + col * (colW + 18), row * 28, colW, 28 }, a, i + 1));
    }
}

void MatrixBank::refresh()
{
    for (auto* r : rows_) r->refresh();
}
}

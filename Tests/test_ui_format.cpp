// 08-09 console UI: value formatting, names/hints, mod destination table,
// snapshot live state, and an editor smoke test (every continuous parameter
// has a labelled control; the canvas scales with a fixed ratio).

#include <catch2/catch_test_macros.hpp>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "UI/ParamFormat.h"
#include "UI/Console.h"
#include "UI/Monitor.h"
#include "UI/Header.h"
#include "UI/ConsoleView.h"
#include <set>

namespace
{
    const juce::String minus = juce::String::fromUTF8("\xe2\x88\x92");
}

TEST_CASE("ParamFormat shows values with units", "[ui]")
{
    using ParamFormat::valueText;
    CHECK(valueText("grainSize", 100.0f) == "100 ms");
    CHECK(valueText("grainSize", 7.5f) == "7.5 ms");
    CHECK(valueText("density", 42.0f) == "42 g/s");
    CHECK(valueText("position", 0.45f) == "45 %");
    CHECK(valueText("pitchShift", 7.0f) == "+7.0 st");
    CHECK(valueText("pitchShift", 0.0f) == "0.0 st");
    CHECK(valueText("pitchShift", -12.0f) == minus + "12.0 st");
    CHECK(valueText("masterVolume", -6.0f) == minus + "6.0 dB");
    CHECK(valueText("masterVolume", -60.0f) == minus + juce::String::fromUTF8("\xe2\x88\x9e dB"));
    CHECK(valueText("crackleLevel", -60.0f) == "off");
    CHECK(valueText("lfo1Rate", 8000.0f) == "8.00 kHz");
    CHECK(valueText("lfo2Rate", 0.5f) == "0.50 Hz");
    CHECK(valueText("wowRate", 12.34f) == "12.3 Hz");
    CHECK(valueText("tiltEq", -0.25f) == "dark 25");
    CHECK(valueText("tiltEq", 0.0f) == "flat");
    CHECK(valueText("slot3Amount", -0.5f) == minus + "50 %");
    CHECK(valueText("slot1Source", 2.0f) == "LFO 2");
    CHECK(valueText("triggerMode", 2.0f) == "Euclid");
    CHECK(valueText("euclidSteps", 13.0f) == "13");
    CHECK(valueText("motionEnabled", 1.0f) == "on");
}

TEST_CASE("ParamFormat splits number and unit", "[ui]")
{
    auto p = ParamFormat::split("120 ms");
    CHECK(p.number == "120");
    CHECK(p.unit == "ms");
    p = ParamFormat::split("flat");
    CHECK(p.number == "flat");
    CHECK(p.unit.isEmpty());
    p = ParamFormat::split("dark 25");
    CHECK(p.unit.isEmpty());
    p = ParamFormat::split(minus + juce::String::fromUTF8("\xe2\x88\x9e dB"));
    CHECK(p.unit == "dB");
}

TEST_CASE("Every parameter has a name, hint and readable value", "[ui]")
{
    GranoAudioProcessor proc;
    for (auto* p : proc.getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p);
        REQUIRE(rp != nullptr);
        const auto id = rp->getParameterID();
        INFO(id);
        CHECK(ParamFormat::fullName(id) != id);
        CHECK(ParamFormat::hint(id).isNotEmpty());
        CHECK(ParamFormat::valueText(id, rp->convertFrom0to1(rp->getDefaultValue())).isNotEmpty());
    }
}

TEST_CASE("Mod destination table matches the parameter layout", "[ui]")
{
    GranoAudioProcessor proc;
    auto* dest = dynamic_cast<juce::AudioParameterChoice*>(proc.getAPVTS().getParameter("slot1Dest"));
    REQUIRE(dest != nullptr);
    REQUIRE(dest->choices.size() == ParamFormat::kNumModDests);
    CHECK(ParamFormat::modDestParamID(0).isEmpty());
    for (int i = 1; i < ParamFormat::kNumModDests; ++i)
    {
        INFO(dest->choices[i]);
        CHECK(proc.getAPVTS().getParameter(ParamFormat::modDestParamID(i)) != nullptr);
    }
}

TEST_CASE("Snapshots track the live slot", "[ui][snapshots]")
{
    GranoAudioProcessor proc;
    auto& s = proc.getSnapshots();
    CHECK(s.getLive() == -1);
    s.save(1, proc.getAPVTS().copyState());
    CHECK(s.getLive() == 1);
    s.save(2, proc.getAPVTS().copyState());
    s.recall(1, proc.getAPVTS());
    CHECK(s.getLive() == 1);
    s.clearSlot(1);
    CHECK_FALSE(s.isOccupied(1));
    CHECK(s.getLive() == -1);
    CHECK(s.isOccupied(2));
}

TEST_CASE("Editor shows every continuous parameter and scales with fixed ratio", "[ui][editor]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    GranoAudioProcessor proc;
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
        REQUIRE(ed != nullptr);
        CHECK(ed->getWidth() == 1200);
        CHECK(ed->getHeight() == 780);

        std::vector<Console::ParamControl*> controls;
        Console::collectControls(*ed, controls);
        std::set<juce::String> ids;
        for (auto* c : controls)
        {
            ids.insert(c->getParam().getParameterID());
            CHECK(c->getTitle().isNotEmpty());
            CHECK(c->getTooltip().isNotEmpty());
        }
        for (auto* p : proc.getParameters())
            if (auto* f = dynamic_cast<juce::AudioParameterFloat*>(p))
                if (f->getParameterID() != "envelopeShape")   // shape keys
                {
                    INFO(f->getParameterID());
                    CHECK(ids.count(f->getParameterID()) == 1);
                }

        ed->setSize(900, 585);
        CHECK(ed->getWidth() == 900);
        ed->setSize(1920, 1248);
        CHECK(ed->getHeight() == 1248);
    }
}

// Hidden: renders the editor to PNGs for visual review / docs.
//   GranoTests "[screenshot]"  (writes to $GRANO_SHOT_DIR or the temp dir)
TEST_CASE("Editor screenshots", "[.][screenshot]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto outDir = juce::SystemStats::getEnvironmentVariable("GRANO_SHOT_DIR",
                            juce::File::getSpecialLocation(juce::File::tempDirectory).getFullPathName());

    auto writeWav = [](const juce::String& name, int frames, int channels, float f)
    {
        auto file = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(name);
        file.deleteFile();
        juce::AudioBuffer<float> buf(channels, frames);
        juce::Random rng(7);
        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < frames; ++i)
            {
                const float env = 0.35f + 0.45f * std::abs(std::sin((float)i / (float)frames * 23.0f));
                buf.setSample(ch, i, env * (0.6f * std::sin(f * (float)i) + 0.4f * (rng.nextFloat() * 2.0f - 1.0f)));
            }
        juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatWriter> w(format.createWriterFor(
            new juce::FileOutputStream(file), 48000.0, (unsigned)channels, 16, {}, 0));
        w->writeFromAudioSampleBuffer(buf, 0, frames);
        return file;
    };

    auto shoot = [&](GranoAudioProcessor& proc, const juce::String& name)
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
        juce::AudioBuffer<float> io(2, 512);
        juce::MidiBuffer midi;
        for (int i = 0; i < 40; ++i) { io.clear(); proc.processBlock(io, midi); }
        // No message loop in tests: drive the 30 Hz telemetry by hand.
        std::function<void(juce::Component&)> tick = [&](juce::Component& c)
        {
            if (auto* m = dynamic_cast<Monitor*>(&c)) m->tick();
            if (auto* sb = dynamic_cast<SnapshotBar*>(&c)) sb->refresh();
            for (auto* ch : c.getChildren()) tick(*ch);
        };
        tick(*ed);
        std::function<void(juce::Component&)> telemetry = [&](juce::Component& c)
        {
            if (auto* v = dynamic_cast<ConsoleView*>(&c))
                for (int i = 0; i < 4; ++i) { proc.processBlock(io, midi); v->tickTelemetry(); }
            for (auto* ch : c.getChildren()) telemetry(*ch);
        };
        telemetry(*ed);
        const auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 2.0f);
        juce::File out = juce::File(outDir).getChildFile(name);
        out.deleteFile();
        juce::FileOutputStream os(out);
        juce::PNGImageFormat().writeImageToStream(img, os);
    };

    {
        GranoAudioProcessor proc;
        proc.prepareToPlay(48000.0, 512);
        proc.loadSampleFile(writeWav("field-rain.wav", 115680, 2, 0.013f), 0);
        shoot(proc, "grano-default.png");
    }
    {
        GranoAudioProcessor proc;
        proc.prepareToPlay(48000.0, 512);
        proc.loadSampleFile(writeWav("field-rain.wav", 115680, 2, 0.013f), 0);
        proc.loadSampleFile(writeWav("glass-bowl.aif.wav", 56640, 2, 0.05f), 1);
        proc.loadSampleFile(writeWav("choir-pad.wav", 288960, 1, 0.004f), 2);
        proc.loadSampleFile(writeWav("metal-scrape-long-name.wav", 161760, 2, 0.11f), 3);
        auto& a = proc.getAPVTS();
        const std::vector<std::pair<const char*, float>> v{
            { "grainSize", 220 }, { "density", 85 }, { "position", 0.38f }, { "positionJitter", 0.22f },
            { "pitchShift", -5 }, { "stereoSpread", 0.8f }, { "masterVolume", -3.2f }, { "loop", 1 },
            { "subGrainDepth", 2 }, { "stochasticDist", 5 }, { "feedbackEnabled", 1 }, { "feedbackGain", 0.62f },
            { "feedbackDamp", 0.45f }, { "spectralEnabled", 1 }, { "spectralMode", 1 }, { "spectralBlurAmount", 0.35f },
            { "slot1Weight", 0.6f }, { "slot2Weight", 0.35f }, { "slot3Weight", 0.8f }, { "envelopeShape", 1 },
            { "motionEnabled", 1 }, { "wowDepth", 0.3f }, { "wowRate", 0.4f }, { "flutterDepth", 0.15f },
            { "flutterRate", 9 }, { "driftAmount", 0.25f }, { "crackleLevel", -34 }, { "crackleColor", 0.6f },
            { "colorEnabled", 1 }, { "saturate", 0.4f }, { "decimate", 0.12f }, { "tiltEq", -0.25f }, { "verbMix", 0.35f },
            { "patternEnabled", 1 }, { "triggerMode", 2 }, { "syncDivision", 2 }, { "euclidPulses", 5 },
            { "euclidSteps", 13 }, { "euclidRotation", 2 }, { "probability", 0.85f }, { "reverseProb", 0.3f },
            { "quantizeScale", 8 }, { "spray", 0.4f }, { "lfo1Rate", 0.6f }, { "lfo1Depth", 0.7f },
            { "lfo2Rate", 6.5f }, { "lfo2Waveform", 5 }, { "lfo2Depth", 0.5f }, { "lfo2Sync", 1 },
            { "slot1Source", 1 }, { "slot1Dest", 3 }, { "slot1Amount", 0.35f }, { "slot2Source", 2 }, { "slot2Dest", 5 },
            { "slot2Amount", -0.5f }, { "slot3Source", 1 }, { "slot3Dest", 1 }, { "slot3Amount", 0.4f },
            { "slot4Source", 2 }, { "slot4Dest", 18 }, { "slot4Amount", 0.6f }, { "slot5Source", 1 }, { "slot5Dest", 6 },
            { "slot5Amount", 0.8f }, { "slot6Source", 2 }, { "slot6Dest", 15 }, { "slot6Amount", 0.25f } };
        for (auto& [id, val] : v)
            if (auto* p = a.getParameter(id))
                p->setValueNotifyingHost(p->convertTo0to1(val));
        proc.getSnapshots().save(0, a.copyState());
        proc.getSnapshots().save(1, a.copyState());
        proc.getSnapshots().save(2, a.copyState());
        auto set = [&](const char* id, float val)
        {
            auto* p = a.getParameter(id);
            p->setValueNotifyingHost(p->convertTo0to1(val));
        };
        shoot(proc, "grano-dense.png");

        // Pattern modes (Euclid is the dense shot above).
        set("triggerMode", 1); shoot(proc, "grano-pattern-sync.png");
        set("triggerMode", 3); shoot(proc, "grano-pattern-audio.png");
        set("triggerMode", 0); shoot(proc, "grano-pattern-free.png");
        set("patternEnabled", 0); shoot(proc, "grano-dense-free.png");

        // Hero: Free trigger, sample source (no feedback / spectral), so grains
        // spread across the monitor.
        set("patternEnabled", 1);
        set("feedbackEnabled", 0); set("spectralEnabled", 0);
        set("positionJitter", 0.55f); set("density", 60);
        shoot(proc, "grano-hero.png");
    }
}

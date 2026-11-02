// Acceptance-test automation (TESTING.md) — headless checks on the real
// GranoAudioProcessor. Hidden from the default run ([.] tag); run with
//
//   GranoTests "[qa]" -r junit -o qa.xml        (GRANO_QA_DIR = metrics dir)
//
// Every test case is named "QA <id> ..." where <id> is the TESTING.md test it
// covers. Tagged [auto] when the check fully covers the TESTING.md expectation
// (no human needed), [pre] when it only proves the engine side and a person
// must still listen or look. Measured numbers (CPU, RAM) are appended to
// $GRANO_QA_DIR/metrics.txt as "id key=value" lines.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "PluginProcessor.h"
#include "Parameters.h"

#include <cmath>
#include <cstdlib>
#include <functional>

#if JUCE_MAC
 #include <mach/mach.h>
#elif JUCE_WINDOWS
 #include <windows.h>
 #include <psapi.h>
 #pragma comment(lib, "psapi.lib")
#elif JUCE_LINUX
 #include <unistd.h>
#endif

namespace
{
constexpr double kSR    = 48000.0;
constexpr int    kBlock = 512;

// ── metrics + memory ─────────────────────────────────────────────────────────

void metric(const juce::String& id, const juce::String& key, double value)
{
    const char* dir = std::getenv("GRANO_QA_DIR");
    if (dir == nullptr || *dir == 0)
        return;
    juce::File(dir).getChildFile("metrics.txt")
        .appendText(id + " " + key + "=" + juce::String(value, 2) + "\n");
}

double residentMB()
{
#if JUCE_MAC
    mach_task_basic_info info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS)
        return -1.0;
    return (double)info.resident_size / (1024.0 * 1024.0);
#elif JUCE_WINDOWS
    PROCESS_MEMORY_COUNTERS pmc{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return -1.0;
    return (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
#elif JUCE_LINUX
    long pages = 0, resident = 0;
    if (FILE* f = std::fopen("/proc/self/statm", "r"))
    {
        if (std::fscanf(f, "%ld %ld", &pages, &resident) != 2)
            resident = 0;
        std::fclose(f);
    }
    return (double)resident * (double)sysconf(_SC_PAGESIZE) / (1024.0 * 1024.0);
#else
    return -1.0;
#endif
}

// ── test material ────────────────────────────────────────────────────────────

juce::File tempFile(const juce::String& name)
{
    auto f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(name);
    f.deleteFile();
    return f;
}

// Mono or stereo 220 Hz sine with a little noise, written with `format`.
juce::File writeSample(juce::AudioFormat& format, const juce::String& name,
                       double seconds, int channels = 1, int bits = 16)
{
    const auto file = tempFile(name);
    const int frames = (int)(seconds * 44100.0);
    juce::AudioBuffer<float> buf(channels, frames);
    juce::Random rng(1234);
    for (int i = 0; i < frames; ++i)
    {
        const float s = 0.5f * std::sin(juce::MathConstants<float>::twoPi * 220.0f * (float)i / 44100.0f)
                      + 0.02f * (rng.nextFloat() - 0.5f);
        for (int c = 0; c < channels; ++c)
            buf.setSample(c, i, s);
    }
    std::unique_ptr<juce::AudioFormatWriter> w(format.createWriterFor(
        new juce::FileOutputStream(file), 44100.0, (unsigned)channels, bits, {}, 0));
    REQUIRE(w != nullptr);
    w->writeFromAudioSampleBuffer(buf, 0, frames);
    return file;
}

juce::File defaultSample()
{
    static juce::File f = [] {
        juce::WavAudioFormat wav;
        return writeSample(wav, "grano_qa_sine.wav", 4.0);
    }();
    return f;
}

// ── render statistics ────────────────────────────────────────────────────────

struct Stats
{
    double rmsL = 0, rmsR = 0, peak = 0, maxDiffLR = 0, corr = 0;
    double zcrPerSec = 0;    // zero crossings / s on L (pitch proxy)
    double secondsCpu = 0;   // wall time spent in processBlock
    double seconds = 0;      // audio rendered
    bool   finite = true;
    double rms() const { return std::sqrt(0.5 * (rmsL * rmsL + rmsR * rmsR)); }
    double cpuPct() const { return seconds > 0 ? 100.0 * secondsCpu / seconds : 0; }
};

struct Rig
{
    GranoAudioProcessor proc;
    juce::MidiBuffer    midi;
    juce::AudioBuffer<float> buf{ 2, kBlock };

    Rig() { proc.prepareToPlay(kSR, kBlock); }
    ~Rig() { proc.releaseResources(); }

    juce::RangedAudioParameter* param(const char* id)
    {
        auto* p = proc.getAPVTS().getParameter(id);
        REQUIRE(p != nullptr);
        return p;
    }
    void set(const char* id, float plain) { auto* p = param(id); p->setValueNotifyingHost(p->convertTo0to1(plain)); }
    float get(const char* id) { return proc.getAPVTS().getRawParameterValue(id)->load(); }

    void load(const juce::File& f, int slot = 0)
    {
        proc.loadSampleFile(f, slot);
        REQUIRE(proc.getLastLoadError().isEmpty());
        settle();
    }

    // Swap the loaded buffers in and run the spectral update (headless: no timer).
    void settle()
    {
        buf.clear();
        proc.processBlock(buf, midi);
        proc.applyPendingSpectralUpdate();
    }

    // Render `seconds`; skip the first `warm` seconds in the statistics.
    // `perBlock` runs before each block (automation surrogate).
    Stats render(double seconds, double warm = 0.5,
                 const std::function<void(int)>& perBlock = {})
    {
        Stats s;
        const int warmBlocks = (int)(warm * kSR / kBlock);
        const int blocks     = (int)(seconds * kSR / kBlock);
        double sumL = 0, sumR = 0, sumLR = 0;
        long long n = 0, crossings = 0;
        float prev = 0.0f;
        for (int b = 0; b < warmBlocks + blocks; ++b)
        {
            if (perBlock) perBlock(b);
            buf.clear();
            const auto t0 = juce::Time::getHighResolutionTicks();
            proc.processBlock(buf, midi);
            const auto t1 = juce::Time::getHighResolutionTicks();
            if (b < warmBlocks)
                continue;
            s.secondsCpu += juce::Time::highResolutionTicksToSeconds(t1 - t0);
            const float* L = buf.getReadPointer(0);
            const float* R = buf.getReadPointer(1);
            for (int i = 0; i < kBlock; ++i)
            {
                if (!std::isfinite(L[i]) || !std::isfinite(R[i])) { s.finite = false; continue; }
                sumL += (double)L[i] * L[i];
                sumR += (double)R[i] * R[i];
                sumLR += (double)L[i] * R[i];
                s.peak = std::max(s.peak, (double)std::max(std::abs(L[i]), std::abs(R[i])));
                s.maxDiffLR = std::max(s.maxDiffLR, (double)std::abs(L[i] - R[i]));
                if ((prev < 0.0f) != (L[i] < 0.0f)) ++crossings;
                prev = L[i];
                ++n;
            }
        }
        if (n > 0)
        {
            s.rmsL = std::sqrt(sumL / (double)n);
            s.rmsR = std::sqrt(sumR / (double)n);
            const double den = std::sqrt(sumL * sumR);
            s.corr = den > 0 ? sumLR / den : 1.0;
        }
        s.seconds   = (double)blocks * kBlock / kSR;
        s.zcrPerSec = s.seconds > 0 ? (double)crossings / s.seconds : 0;
        return s;
    }
};

void requireHealthy(const Stats& s, double maxPeak = 4.0)
{
    REQUIRE(s.finite);
    REQUIRE(s.peak < maxPeak);   // no blow-up (+12 dBFS ceiling)
}

void requireAudible(const Stats& s)
{
    requireHealthy(s);
    REQUIRE(s.rms() > 1e-4);
}
} // namespace

// ═════════════════════════════════════════════════════════════════════════════
// 1 · First launch
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("QA 1.2 default parameter values", "[.][qa][pre]")
{
    Rig r;
    CHECK(r.get(ParamIDs::grainSize)      == Catch::Approx(100.0f));
    CHECK(r.get(ParamIDs::density)        == Catch::Approx(10.0f));
    CHECK(r.get(ParamIDs::position)       == Catch::Approx(0.0f));
    CHECK(r.get(ParamIDs::positionJitter) == Catch::Approx(0.0f));
    CHECK(r.get(ParamIDs::pitchShift)     == Catch::Approx(0.0f));
    CHECK(r.get(ParamIDs::stereoSpread)   == Catch::Approx(0.5f));
    CHECK(r.get(ParamIDs::masterVolume)   == Catch::Approx(0.0f));
    CHECK(r.get(ParamIDs::loop)           == 0.0f);
    CHECK(r.get(ParamIDs::motionEnabled)  == 0.0f);
    CHECK(r.get(ParamIDs::colorEnabled)   == 0.0f);
    CHECK(r.get(ParamIDs::patternEnabled) == 0.0f);
    CHECK(r.get(ParamIDs::lfo1Rate)       == Catch::Approx(1.0f));
    CHECK(r.get(ParamIDs::lfo2Rate)       == Catch::Approx(1.0f));
    CHECK(r.get(ParamIDs::lfo1Waveform)   == 0.0f);
    CHECK(r.get(ParamIDs::lfo2Waveform)   == 0.0f);
    CHECK(r.get(ParamIDs::lfo1Depth)      == Catch::Approx(0.5f));
    CHECK(r.get(ParamIDs::lfo2Depth)      == Catch::Approx(0.5f));
    CHECK(r.get(ParamIDs::subGrainDepth)  == 0.0f);
    CHECK(r.get(ParamIDs::stochasticDist) == 0.0f);
    CHECK(r.get(ParamIDs::feedbackEnabled) == 0.0f);
    CHECK(r.get(ParamIDs::spectralEnabled) == 0.0f);
    for (const char* id : { ParamIDs::slot1Source, ParamIDs::slot2Source, ParamIDs::slot3Source,
                            ParamIDs::slot4Source, ParamIDs::slot5Source, ParamIDs::slot6Source,
                            ParamIDs::slot7Source, ParamIDs::slot8Source })
        CHECK(r.get(id) == 0.0f);
    for (int s = 0; s < 4; ++s)
        CHECK_FALSE(r.proc.getSnapshots().isOccupied(s));
}

// ═════════════════════════════════════════════════════════════════════════════
// 2 · Sample loading
// ═════════════════════════════════════════════════════════════════════════════

namespace
{
void checkFormatLoads(juce::AudioFormat& fmt, const juce::String& ext, int slot)
{
    const auto f = writeSample(fmt, "grano_qa_fmt" + ext, 2.0, 2);
    Rig r;
    r.load(f, slot);
    const auto info = r.proc.getSlotInfo(slot);
    REQUIRE(info.numFrames > 0);
    REQUIRE(r.proc.getSampleBank().getNumSamples(slot) == info.numFrames);
    r.set(slot == 0 ? ParamIDs::slot0Weight : ParamIDs::slot1Weight, 1.0f);
    requireAudible(r.render(1.0));
    f.deleteFile();
}
} // namespace

TEST_CASE("QA 2.1 WAV loads and plays", "[.][qa][pre]")  { juce::WavAudioFormat f;  checkFormatLoads(f, ".wav", 0); }
TEST_CASE("QA 2.2 AIFF loads and plays", "[.][qa][pre]") { juce::AiffAudioFormat f; checkFormatLoads(f, ".aiff", 1); }
TEST_CASE("QA 2.3 FLAC loads and plays", "[.][qa][pre]") { juce::FlacAudioFormat f; checkFormatLoads(f, ".flac", 0); }
TEST_CASE("QA 2.4 OGG loads and plays", "[.][qa][pre]")  { juce::OggVorbisAudioFormat f; checkFormatLoads(f, ".ogg", 0); }

TEST_CASE("QA 2.6 invalid file rejected", "[.][qa][auto]")
{
    const auto f = tempFile("grano_qa_not_audio.wav");
    f.replaceWithText("this is not audio");
    Rig r;
    r.proc.loadSampleFile(f, 0);
    REQUIRE_FALSE(r.proc.getLastLoadError().isEmpty());
    r.settle();
    REQUIRE(r.proc.getSampleBank().getNumSamples(0) == 0);
    requireHealthy(r.render(0.5)); // still running (test tone), no crash
    f.deleteFile();
}

TEST_CASE("QA 2.7 audio after sample load", "[.][qa][pre]")
{
    Rig r;
    r.load(defaultSample());
    requireAudible(r.render(2.0));
}

// ═════════════════════════════════════════════════════════════════════════════
// 3 · Core engine
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("QA 3.1-3.6 grain size, density, position, jitter extremes", "[.][qa][pre]")
{
    struct Case { const char* id; float value; };
    for (const auto& c : { Case{ ParamIDs::grainSize, 5.0f }, Case{ ParamIDs::grainSize, 500.0f },
                           Case{ ParamIDs::density, 1.0f },   Case{ ParamIDs::density, 100.0f },
                           Case{ ParamIDs::position, 1.0f },  Case{ ParamIDs::positionJitter, 1.0f } })
    {
        INFO(c.id << " = " << c.value);
        Rig r;
        r.load(defaultSample());
        r.set(c.id, c.value);
        // Density 1 g/s needs a longer window to catch grains.
        requireAudible(r.render(c.value == 1.0f ? 4.0 : 2.0));
    }
}

TEST_CASE("QA 3.7-3.9 pitch shift moves pitch by octaves", "[.][qa][pre]")
{
    auto zcr = [](float st) {
        Rig r;
        r.load(defaultSample());
        r.set(ParamIDs::stereoSpread, 0.0f);
        r.set(ParamIDs::density, 40.0f);
        r.set(ParamIDs::grainSize, 200.0f);
        r.set(ParamIDs::pitchShift, st);
        const auto s = r.render(3.0);
        requireAudible(s);
        return s.zcrPerSec;
    };
    const double z0 = zcr(0.0f), zUp = zcr(24.0f), zDown = zcr(-24.0f);
    INFO("zero crossings/s  0st=" << z0 << "  +24st=" << zUp << "  -24st=" << zDown);
    REQUIRE(z0 > 100.0);                               // grain overlap adds crossings: ratios only
    CHECK(zUp / z0 > 2.5);                             // ideal 4×
    CHECK(zDown / z0 < 0.45);                          // ideal 0.25×
}

TEST_CASE("QA 3.10-3.11 stereo spread mono vs wide", "[.][qa][pre]")
{
    Rig mono;
    mono.load(defaultSample());
    mono.set(ParamIDs::stereoSpread, 0.0f);
    const auto m = mono.render(2.0);
    requireAudible(m);
    CHECK(m.maxDiffLR < 1e-4);

    Rig wide;
    wide.load(defaultSample());
    wide.set(ParamIDs::stereoSpread, 1.0f);
    wide.set(ParamIDs::density, 30.0f);
    const auto w = wide.render(2.0);
    requireAudible(w);
    INFO("L/R correlation at 100% = " << w.corr);
    CHECK(w.corr < 0.9);
}

TEST_CASE("QA 3.12-3.13 master volume -60 dB and +6 dB", "[.][qa][pre]")
{
    auto level = [](float db) {
        Rig r;
        r.load(defaultSample());
        r.set(ParamIDs::density, 40.0f);
        r.set(ParamIDs::masterVolume, db);
        const auto s = r.render(4.0, 1.0);
        requireHealthy(s);
        return s.rms();
    };
    const double ref = level(0.0f), quiet = level(-60.0f), loud = level(6.0f);
    INFO("rms 0dB=" << ref << " -60dB=" << quiet << " +6dB=" << loud);
    REQUIRE(ref > 1e-4);
    CHECK(quiet / ref < 0.003);     // ideal 0.001
    CHECK(loud / ref > 1.4);        // ideal 2.0 (grain randomness between runs)
    CHECK(loud / ref < 2.8);
}

TEST_CASE("QA 3.14-3.15 loop on and off", "[.][qa][pre]")
{
    Rig r;
    r.load(defaultSample());
    r.set(ParamIDs::loop, 1.0f);
    requireAudible(r.render(3.0));
    r.set(ParamIDs::loop, 0.0f);
    requireAudible(r.render(1.0));
}

// ═════════════════════════════════════════════════════════════════════════════
// 4–6 · Modules
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("QA 4.1-4.8 MOTION on, every control at maximum", "[.][qa][pre]")
{
    Rig r;
    r.load(defaultSample());
    r.set(ParamIDs::motionEnabled, 1.0f);
    for (const char* id : { ParamIDs::wowDepth, ParamIDs::wowRate, ParamIDs::flutterDepth,
                            ParamIDs::flutterRate, ParamIDs::driftAmount, ParamIDs::crackleLevel,
                            ParamIDs::crackleColor })
        r.param(id)->setValueNotifyingHost(1.0f);
    requireAudible(r.render(3.0));
}

TEST_CASE("QA 5.1-5.7 COLOR on, sweeps at both ends", "[.][qa][pre]")
{
    for (float v : { 0.0f, 1.0f })
    {
        INFO("normalised value " << v);
        Rig r;
        r.load(defaultSample());
        r.set(ParamIDs::colorEnabled, 1.0f);
        for (const char* id : { ParamIDs::saturate, ParamIDs::decimate, ParamIDs::tiltEq, ParamIDs::verbMix })
            r.param(id)->setValueNotifyingHost(v);
        requireAudible(r.render(3.0));
    }
}

TEST_CASE("QA 6.1-6.12 PATTERN Free and Audio modes, probability, reverse, spray", "[.][qa][pre]")
{
    for (int mode : { 0, 3 })   // Sync/Euclid need a host transport: manual
    {
        INFO("trigger mode " << mode);
        Rig r;
        r.load(defaultSample());
        r.set(ParamIDs::patternEnabled, 1.0f);
        r.set(ParamIDs::triggerMode, (float)mode);
        r.set(ParamIDs::probability, 0.7f);
        r.set(ParamIDs::reverseProb, 0.5f);
        r.set(ParamIDs::spray, 1.0f);
        r.set(ParamIDs::quantizeScale, 2.0f);
        requireHealthy(r.render(3.0));
    }
}

TEST_CASE("QA 6.11 all 11 quantize scales enumerate", "[.][qa][auto]")
{
    Rig r;
    auto* p = dynamic_cast<juce::AudioParameterChoice*>(r.param(ParamIDs::quantizeScale));
    REQUIRE(p != nullptr);
    REQUIRE(p->choices.size() == 11);
    r.load(defaultSample());
    r.set(ParamIDs::patternEnabled, 1.0f);
    for (int i = 0; i < 11; ++i)
    {
        INFO("scale " << p->choices[i]);
        r.set(ParamIDs::quantizeScale, (float)i);
        r.set(ParamIDs::pitchShift, 7.3f);
        requireHealthy(r.render(0.5, 0.0));
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 7–9 · LFO, matrix, snapshots
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("QA 7.1-7.12 every LFO waveform modulating grain size", "[.][qa][pre]")
{
    for (int wf = 0; wf < 6; ++wf)
    {
        INFO("waveform " << wf);
        Rig r;
        r.load(defaultSample());
        r.set(ParamIDs::lfo1Waveform, (float)wf);
        r.set(ParamIDs::lfo1Rate, 4.0f);
        r.set(ParamIDs::lfo1Depth, 1.0f);
        r.set(ParamIDs::slot1Source, 1.0f);   // LFO1
        r.set(ParamIDs::slot1Dest, 1.0f);     // Grain Size
        r.set(ParamIDs::slot1Amount, 1.0f);
        requireAudible(r.render(2.0));
    }
}

TEST_CASE("QA 8.4 matrix lists 29 destinations", "[.][qa][pre]")
{
    Rig r;
    auto* p = dynamic_cast<juce::AudioParameterChoice*>(r.param(ParamIDs::slot1Dest));
    REQUIRE(p != nullptr);
    REQUIRE(p->choices.size() == 29);
}

TEST_CASE("QA 8.3 all 8 matrix slots active", "[.][qa][pre]")
{
    Rig r;
    r.load(defaultSample());
    const char* src[] = { ParamIDs::slot1Source, ParamIDs::slot2Source, ParamIDs::slot3Source, ParamIDs::slot4Source,
                          ParamIDs::slot5Source, ParamIDs::slot6Source, ParamIDs::slot7Source, ParamIDs::slot8Source };
    const char* dst[] = { ParamIDs::slot1Dest, ParamIDs::slot2Dest, ParamIDs::slot3Dest, ParamIDs::slot4Dest,
                          ParamIDs::slot5Dest, ParamIDs::slot6Dest, ParamIDs::slot7Dest, ParamIDs::slot8Dest };
    const char* amt[] = { ParamIDs::slot1Amount, ParamIDs::slot2Amount, ParamIDs::slot3Amount, ParamIDs::slot4Amount,
                          ParamIDs::slot5Amount, ParamIDs::slot6Amount, ParamIDs::slot7Amount, ParamIDs::slot8Amount };
    for (int i = 0; i < 8; ++i)
    {
        r.set(src[i], (float)(1 + i % 2));
        r.set(dst[i], (float)(1 + i));       // Grain Size … Wow Depth
        r.set(amt[i], i % 2 ? -0.6f : 0.6f);
    }
    requireAudible(r.render(3.0));
}

TEST_CASE("QA 9.1-9.4 snapshots store, recall and persist", "[.][qa][pre]")
{
    const float sizes[] = { 20.0f, 80.0f, 200.0f, 400.0f };
    juce::MemoryBlock state;
    {
        Rig r;
        auto& snaps = r.proc.getSnapshots();
        for (int i = 0; i < 4; ++i)
        {
            r.set(ParamIDs::grainSize, sizes[i]);
            snaps.save(i, r.proc.getAPVTS().copyState());
        }
        for (int i = 3; i >= 0; --i)
        {
            snaps.recall(i, r.proc.getAPVTS());
            CHECK(r.get(ParamIDs::grainSize) == Catch::Approx(sizes[i]).margin(0.5f));
        }
        r.proc.getStateInformation(state);
    }
    Rig restored;
    restored.proc.setStateInformation(state.getData(), (int)state.getSize());
    auto& snaps = restored.proc.getSnapshots();
    for (int i = 0; i < 4; ++i)
    {
        REQUIRE(snaps.isOccupied(i));
        snaps.recall(i, restored.proc.getAPVTS());
        CHECK(restored.get(ParamIDs::grainSize) == Catch::Approx(sizes[i]).margin(0.5f));
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 10–13 · Sub-grain, stochastic, feedback, spectral
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("QA 10.1-10.3 sub-grain depth 0, 1, 2", "[.][qa][pre]")
{
    for (int d = 0; d <= 2; ++d)
    {
        INFO("depth " << d);
        Rig r;
        r.load(defaultSample());
        r.set(ParamIDs::subGrainDepth, (float)d);
        requireAudible(r.render(2.0));
    }
}

TEST_CASE("QA 11.1-11.7 all six timing distributions, switched live", "[.][qa][pre]")
{
    Rig r;
    r.load(defaultSample());
    r.set(ParamIDs::density, 30.0f);
    for (int d = 0; d < 6; ++d)
    {
        INFO("distribution " << d);
        r.set(ParamIDs::stochasticDist, (float)d);
        requireAudible(r.render(1.5, 0.0));
    }
}

TEST_CASE("QA 12.1 feedback on keeps producing audio", "[.][qa][pre]")
{
    // TESTING 12.1: a sample is playing, Feedback is switched on (gain 0.5).
    // The loop must keep sounding, not fade to silence.
    Rig r;
    r.load(defaultSample());
    r.set(ParamIDs::grainSize, 150.0f);
    requireAudible(r.render(2.0));
    r.set(ParamIDs::feedbackEnabled, 1.0f);
    r.set(ParamIDs::feedbackGain, 0.5f);
    (void)r.render(2.0, 0.0);
    const auto s = r.render(8.0, 0.0);
    INFO("rms with feedback on, seconds 2-10: " << s.rms());
    requireAudible(s);
}

TEST_CASE("QA 12.3 feedback at maximum gain is stable", "[.][qa][auto]")
{
    Rig r;
    r.load(defaultSample());
    r.set(ParamIDs::feedbackEnabled, 1.0f);
    r.set(ParamIDs::feedbackGain, 0.95f);
    r.set(ParamIDs::feedbackDamp, 0.0f);
    r.set(ParamIDs::density, 60.0f);
    const auto early = r.render(5.0);
    const auto late  = r.render(55.0, 0.0);
    requireAudible(early);
    requireAudible(late);
    INFO("rms first 5 s=" << early.rms() << "  next 55 s=" << late.rms() << "  peak=" << late.peak);
    CHECK(late.rms() < std::max(0.05, early.rms() * 8.0)); // no runaway growth
}

TEST_CASE("QA 12.1-12.5 feedback on, gain and damp sweep, off", "[.][qa][pre]")
{
    Rig r;
    r.load(defaultSample());
    r.set(ParamIDs::feedbackEnabled, 1.0f);
    for (float g : { 0.0f, 0.5f, 0.95f })
        for (float d : { 0.0f, 1.0f })
        {
            r.set(ParamIDs::feedbackGain, g);
            r.set(ParamIDs::feedbackDamp, d);
            requireHealthy(r.render(1.0, 0.0));
        }
    r.set(ParamIDs::feedbackEnabled, 0.0f);
    requireAudible(r.render(1.0));
}

TEST_CASE("QA 13.1-13.4 spectral freeze and blur", "[.][qa][pre]")
{
    for (int mode : { 0, 1 })
    {
        INFO("spectral mode " << mode);
        Rig r;
        r.load(defaultSample());
        r.set(ParamIDs::spectralEnabled, 1.0f);
        r.set(ParamIDs::spectralMode, (float)mode);
        r.set(ParamIDs::spectralBlurAmount, 0.8f);
        r.settle();
        requireAudible(r.render(2.0));
    }
}

TEST_CASE("QA 13.5-13.6 spectral reports no latency on and off", "[.][qa][auto]")
{
    Rig r;
    r.load(defaultSample());
    r.set(ParamIDs::spectralEnabled, 1.0f);
    r.settle();
    REQUIRE(r.proc.getLatencySamples() == 0);
    r.set(ParamIDs::spectralEnabled, 0.0f);
    r.settle();
    REQUIRE(r.proc.getLatencySamples() == 0);
}

// ═════════════════════════════════════════════════════════════════════════════
// 15 · State persistence
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("QA 15.1-15.2 every parameter and sample survive save and restore", "[.][qa][auto]")
{
    const auto sample = defaultSample();
    juce::MemoryBlock state;
    std::vector<std::pair<juce::String, float>> expected;
    auto text = [](juce::RangedAudioParameter* p, float v) { return p->getText(v, 64); };
    {
        Rig r;
        r.load(sample, 2);
        juce::Random rng(99);
        for (auto* p : r.proc.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
                rp->setValueNotifyingHost(rng.nextFloat());
        for (auto* p : r.proc.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
                expected.emplace_back(rp->getParameterID(), rp->getValue());
        // Bool and choice parameters snap on restore: compare display text.
        r.proc.getStateInformation(state);
    }
    Rig restored;
    restored.proc.setStateInformation(state.getData(), (int)state.getSize());
    restored.settle();
    for (const auto& [id, v] : expected)
    {
        INFO(id);
        auto* p = restored.param(id.toRawUTF8());
        CHECK(text(p, p->getValue()) == text(p, v));
    }
    CHECK(restored.proc.getSampleBank().getNumSamples(2) > 0);
}

TEST_CASE("QA 15.3 two instances keep independent state", "[.][qa][auto]")
{
    Rig a, b;
    a.set(ParamIDs::grainSize, 30.0f);
    b.set(ParamIDs::grainSize, 300.0f);
    a.load(defaultSample(), 0);
    CHECK(a.get(ParamIDs::grainSize) == Catch::Approx(30.0f).margin(0.5f));
    CHECK(b.get(ParamIDs::grainSize) == Catch::Approx(300.0f).margin(0.5f));
    CHECK(a.proc.getSampleBank().getNumSamples(0) > 0);
    b.settle();
    CHECK(b.proc.getSampleBank().getNumSamples(0) == 0);
}

// ═════════════════════════════════════════════════════════════════════════════
// 16 · Performance and stability
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("QA 16.1 CPU at nominal load", "[.][qa][auto][perf]")
{
    Rig r;
    r.load(defaultSample());
    r.set(ParamIDs::density, 100.0f);
    r.set(ParamIDs::grainSize, 50.0f);
    r.set(ParamIDs::motionEnabled, 1.0f);
    r.set(ParamIDs::colorEnabled, 1.0f);
    r.set(ParamIDs::patternEnabled, 1.0f);
    r.set(ParamIDs::feedbackEnabled, 1.0f);
    r.set(ParamIDs::feedbackGain, 0.5f);
    const auto s = r.render(60.0, 1.0);
    metric("16.1", "cpu_pct", s.cpuPct());
    requireAudible(s);
    INFO("CPU " << s.cpuPct() << " % of one core (budget 15 %)");
    CHECK(s.cpuPct() <= 15.0);
}

TEST_CASE("QA 16.2 CPU at worst case", "[.][qa][auto][perf]")
{
    Rig r;
    r.load(defaultSample());
    r.set(ParamIDs::density, 100.0f);
    r.set(ParamIDs::grainSize, 500.0f);    // max density × max size = most overlap
    r.set(ParamIDs::subGrainDepth, 2.0f);
    r.set(ParamIDs::motionEnabled, 1.0f);
    r.set(ParamIDs::colorEnabled, 1.0f);
    r.set(ParamIDs::patternEnabled, 1.0f);
    r.set(ParamIDs::feedbackEnabled, 1.0f);
    r.set(ParamIDs::feedbackGain, 0.5f);
    r.set(ParamIDs::spectralEnabled, 1.0f);
    r.settle();
    const auto s = r.render(30.0, 1.0);
    metric("16.2", "cpu_pct", s.cpuPct());
    requireAudible(s);
    INFO("CPU " << s.cpuPct() << " % of one core (budget 40 %)");
    CHECK(s.cpuPct() <= 40.0);
}

TEST_CASE("QA 16.3 fast modulation and density automation", "[.][qa][auto]")
{
    Rig r;
    r.load(defaultSample());
    r.set(ParamIDs::lfo1Rate, 8.0f);
    r.set(ParamIDs::lfo1Depth, 1.0f);
    r.set(ParamIDs::lfo2Rate, 5.0f);
    r.set(ParamIDs::lfo2Depth, 1.0f);
    r.set(ParamIDs::slot1Source, 1.0f); r.set(ParamIDs::slot1Dest, 1.0f); r.set(ParamIDs::slot1Amount, 1.0f); // → Grain Size
    r.set(ParamIDs::slot2Source, 2.0f); r.set(ParamIDs::slot2Dest, 5.0f); r.set(ParamIDs::slot2Amount, 1.0f); // → Pitch
    const int blocksPerRamp = (int)(8.0 * kSR / kBlock);  // 4 bars at 120 BPM
    auto* density = r.param(ParamIDs::density);
    const auto s = r.render(32.0, 0.0, [&](int b) {
        const float t = (float)(b % blocksPerRamp) / (float)blocksPerRamp;
        density->setValueNotifyingHost(density->convertTo0to1(1.0f + 99.0f * t));
    });
    requireAudible(s);
    metric("16.3", "cpu_pct", s.cpuPct());
}

TEST_CASE("QA 16.4 memory with four large samples", "[.][qa][auto][perf]")
{
    // Four 3-minute stereo 16-bit WAVs (~32 MB each on disk).
    std::vector<juce::File> files;
    juce::WavAudioFormat wav;
    for (int i = 0; i < 4; ++i)
        files.push_back(writeSample(wav, "grano_qa_big" + juce::String(i) + ".wav", 180.0, 2));

    const double before = residentMB();
    {
        Rig r;
        for (int i = 0; i < 4; ++i)
            r.load(files[(size_t)i], i);
        requireHealthy(r.render(2.0));
        const double after = residentMB();
        const double used = after - before;
        metric("16.4", "plugin_mb", used);
        metric("16.4", "process_rss_mb", after);
        INFO("plugin memory " << used << " MB (budget 200 MB); process RSS " << after << " MB");
        if (before >= 0)
            CHECK(used <= 200.0);
    }
    for (auto& f : files) f.deleteFile();
}

TEST_CASE("QA 16.5 rapid bypass toggling", "[.][qa][auto]")
{
    Rig r;
    r.load(defaultSample());
    const char* ids[] = { ParamIDs::motionEnabled, ParamIDs::colorEnabled, ParamIDs::patternEnabled };
    const auto s = r.render(10.0, 0.0, [&](int b) {
        r.param(ids[b % 3])->setValueNotifyingHost((b / 3) % 2 ? 1.0f : 0.0f);
    });
    requireAudible(s);
}

TEST_CASE("QA 16.7 no source: test tone vs silence", "[.][qa][auto]")
{
    Rig tone;
    requireAudible(tone.render(1.0));
    Rig silent;
    silent.set(ParamIDs::noSourceMode, 1.0f);
    const auto s = silent.render(1.0);
    REQUIRE(s.finite);
    REQUIRE(s.peak == 0.0);
}

TEST_CASE("QA 16.8 soak (accelerated)", "[.][qa][auto][perf][soak]")
{
    // TESTING.md 16.8: 30 minutes of audio at moderate settings. Rendered
    // offline (faster than real time); GRANO_QA_SOAK_MIN overrides the length.
    double minutes = 30.0;
    if (const char* m = std::getenv("GRANO_QA_SOAK_MIN"))
        minutes = std::max(0.5, std::atof(m));

    Rig r;
    r.load(defaultSample());
    r.set(ParamIDs::density, 30.0f);
    r.set(ParamIDs::grainSize, 100.0f);
    r.set(ParamIDs::motionEnabled, 1.0f);
    r.set(ParamIDs::colorEnabled, 1.0f);
    r.set(ParamIDs::feedbackEnabled, 1.0f);
    r.set(ParamIDs::feedbackGain, 0.3f);
    (void)r.render(10.0, 0.0);                   // settle allocations
    const double rss0 = residentMB();
    const auto first = r.render(60.0, 0.0);
    const auto rest  = r.render(minutes * 60.0 - 60.0, 0.0);
    const auto last  = r.render(60.0, 0.0);
    const double rss1 = residentMB();

    metric("16.8", "minutes", minutes);
    metric("16.8", "rss_start_mb", rss0);
    metric("16.8", "rss_end_mb", rss1);
    if (first.rms() > 0)
        metric("16.8", "rms_ratio_last_first", last.rms() / first.rms());
    requireAudible(first);
    requireAudible(rest);
    requireAudible(last);
    INFO("RSS " << rss0 << " → " << rss1 << " MB;  rms first/last min " << first.rms() << " / " << last.rms());
    if (rss0 >= 0)
        CHECK(rss1 - rss0 < 8.0);                     // no leak
    CHECK(last.rms() == Catch::Approx(first.rms()).epsilon(0.5)); // no progressive degradation
}

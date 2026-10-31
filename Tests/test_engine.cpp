#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <type_traits>
#include <unordered_set>

#include "Engine/EnvelopeShapes.h"
#include "Engine/Grain.h"
#include "Engine/GrainPool.h"
#include "Engine/GranularEngine.h"

// ── EnvelopeShapes ────────────────────────────────────────────────────────────

static constexpr EnvelopeShape kAllShapes[] = {
    EnvelopeShape::Hann,
    EnvelopeShape::Tukey,
    EnvelopeShape::Gaussian,
    EnvelopeShape::Triangle,
    EnvelopeShape::Rectangle,
};

TEST_CASE("EnvelopeShapes output in [0, 1] for all phases", "[dsp][envelope]")
{
    const float phases[] = { 0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 0.9f, 1.0f };
    for (auto shape : kAllShapes)
    {
        for (float p : phases)
        {
            const float v = applyEnvelope(shape, p);
            REQUIRE(v >= 0.0f);
            REQUIRE(v <= 1.0f);
        }
    }
}

TEST_CASE("EnvelopeShapes boundary values near zero for tapered shapes", "[dsp][envelope]")
{
    // Hann, Tukey, Triangle have hard zeros at boundaries (< 1%).
    const EnvelopeShape hardZero[] = {
        EnvelopeShape::Hann,
        EnvelopeShape::Tukey,
        EnvelopeShape::Triangle,
    };
    for (auto shape : hardZero)
    {
        REQUIRE(applyEnvelope(shape, 0.0f) < 0.01f);
        REQUIRE(applyEnvelope(shape, 1.0f) < 0.01f);
    }

    // Gaussian (σ=0.2 of full window) is a bell curve — boundary value is
    // exp(-0.5 * 2.5^2) ≈ 0.044. Not zero, but substantially below the peak.
    REQUIRE(applyEnvelope(EnvelopeShape::Gaussian, 0.0f) < 0.1f);
    REQUIRE(applyEnvelope(EnvelopeShape::Gaussian, 1.0f) < 0.1f);
}

TEST_CASE("EnvelopeShapes midpoint near peak for tapered shapes", "[dsp][envelope]")
{
    // Hann and Triangle peak exactly at 1.0
    REQUIRE(applyEnvelope(EnvelopeShape::Hann, 0.5f) == Catch::Approx(1.0f).margin(0.01f));
    REQUIRE(applyEnvelope(EnvelopeShape::Triangle, 0.5f) == Catch::Approx(1.0f).margin(0.01f));
    // Tukey is flat in the centre
    REQUIRE(applyEnvelope(EnvelopeShape::Tukey, 0.5f) == Catch::Approx(1.0f).margin(0.01f));
    // Gaussian peaks at 0.5 but value depends on σ
    REQUIRE(applyEnvelope(EnvelopeShape::Gaussian, 0.5f) >= 0.9f);
}

TEST_CASE("Rectangle returns 1.0 for all phases", "[dsp][envelope]")
{
    const float phases[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
    for (float p : phases)
        REQUIRE(applyEnvelope(EnvelopeShape::Rectangle, p) == Catch::Approx(1.0f));
}

TEST_CASE("EnvelopeShapes are symmetric around 0.5", "[dsp][envelope]")
{
    const EnvelopeShape symmetric[] = {
        EnvelopeShape::Hann,
        EnvelopeShape::Gaussian,
        EnvelopeShape::Triangle,
    };
    const float offsets[] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f };
    for (auto shape : symmetric)
    {
        for (float d : offsets)
        {
            const float left  = applyEnvelope(shape, 0.5f - d);
            const float right = applyEnvelope(shape, 0.5f + d);
            REQUIRE(left == Catch::Approx(right).margin(1e-5f));
        }
    }
}

// ── Grain ─────────────────────────────────────────────────────────────────────

TEST_CASE("Grain is trivially copyable", "[grain]")
{
    REQUIRE(std::is_trivially_copyable_v<Grain>);
}

TEST_CASE("Grain stores all fields correctly via aggregate init", "[grain]")
{
    float buf[64] = {};
    Grain g{ buf, 10, 64, 2048, 1.5f, -0.3f, EnvelopeShape::Triangle, 512 };

    REQUIRE(g.source        == buf);
    REQUIRE(g.startPos      == 10);
    REQUIRE(g.sourceLen     == 64);
    REQUIRE(g.lengthSamples == 2048);
    REQUIRE(g.pitchRatio    == Catch::Approx(1.5f));
    REQUIRE(g.pan           == Catch::Approx(-0.3f));
    REQUIRE(g.shape         == EnvelopeShape::Triangle);
    REQUIRE(g.elapsed       == 512);
    REQUIRE(g.phase()       == Catch::Approx(0.25f));
}

TEST_CASE("Grain default init is zero/null", "[grain]")
{
    Grain g{};
    REQUIRE(g.source        == nullptr);
    REQUIRE(g.startPos      == 0);
    REQUIRE(g.sourceLen     == 0);
    REQUIRE(g.lengthSamples == 0);
    REQUIRE(g.pitchRatio    == Catch::Approx(1.0f));
    REQUIRE(g.pan           == Catch::Approx(0.0f));
    REQUIRE(g.elapsed       == 0);
    REQUIRE(g.startDelay    == 0);
    REQUIRE(g.gain          == Catch::Approx(1.0f));
}

// ── GrainPool ─────────────────────────────────────────────────────────────────

TEST_CASE("GrainPool acquires up to capacity", "[pool]")
{
    GrainPool pool;
    std::vector<Grain*> acquired;
    acquired.reserve(GrainPool::Capacity);

    for (int i = 0; i < GrainPool::Capacity; ++i)
    {
        Grain* g = pool.acquire();
        REQUIRE(g != nullptr);
        acquired.push_back(g);
    }

    REQUIRE(pool.activeCount() == GrainPool::Capacity);

    // Clean up
    for (auto* g : acquired)
        pool.release(g);
}

TEST_CASE("GrainPool returns nullptr when exhausted", "[pool]")
{
    GrainPool pool;
    std::vector<Grain*> acquired;
    acquired.reserve(GrainPool::Capacity);

    for (int i = 0; i < GrainPool::Capacity; ++i)
        acquired.push_back(pool.acquire());

    REQUIRE(pool.acquire() == nullptr);

    for (auto* g : acquired)
        pool.release(g);
}

TEST_CASE("GrainPool slot becomes reusable after release", "[pool]")
{
    GrainPool pool;

    // Fill pool
    std::vector<Grain*> acquired;
    for (int i = 0; i < GrainPool::Capacity; ++i)
        acquired.push_back(pool.acquire());

    REQUIRE(pool.acquire() == nullptr);

    // Release one slot
    pool.release(acquired[0]);
    acquired[0] = nullptr;

    // Should be acquirable again
    Grain* reused = pool.acquire();
    REQUIRE(reused != nullptr);

    // Clean up
    pool.release(reused);
    for (auto* g : acquired)
        if (g != nullptr) pool.release(g);
}

TEST_CASE("GrainPool all acquired pointers are distinct", "[pool]")
{
    GrainPool pool;
    std::unordered_set<Grain*> ptrs;

    for (int i = 0; i < GrainPool::Capacity; ++i)
    {
        Grain* g = pool.acquire();
        REQUIRE(g != nullptr);
        REQUIRE(ptrs.find(g) == ptrs.end()); // no duplicates
        ptrs.insert(g);
    }

    for (Grain* g : ptrs)
        pool.release(g);
}

TEST_CASE("GrainPool acquired grain is zero-initialised", "[pool]")
{
    GrainPool pool;
    Grain* g = pool.acquire();
    REQUIRE(g != nullptr);
    REQUIRE(g->source       == nullptr);
    REQUIRE(g->startPos     == 0);
    REQUIRE(g->elapsed      == 0);
    pool.release(g);
}

// ── Pitch ratio math ──────────────────────────────────────────────────────────
// Verifies the source-position formula used in GranularEngine::renderGrain.
// Tests pure math — no live audio required.

static float srcPosAtPhase(float phase, int grainLen, float pitchRatio) noexcept
{
    return phase * (float)grainLen * pitchRatio;
}

TEST_CASE("Pitch ratio 1.0 reads exactly lengthSamples source frames", "[grain][pitch]")
{
    const int N = 200;
    REQUIRE(srcPosAtPhase(0.0f, N, 1.0f) == Catch::Approx(0.0f));
    REQUIRE(srcPosAtPhase(1.0f, N, 1.0f) == Catch::Approx(200.0f));
    REQUIRE(srcPosAtPhase(0.5f, N, 1.0f) == Catch::Approx(100.0f));
}

TEST_CASE("Pitch ratio 2.0 reads twice as many source frames (pitch up)", "[grain][pitch]")
{
    const int N = 200;
    REQUIRE(srcPosAtPhase(1.0f, N, 2.0f) == Catch::Approx(400.0f));
    REQUIRE(srcPosAtPhase(0.5f, N, 2.0f) == Catch::Approx(200.0f));
}

TEST_CASE("Pitch ratio 0.5 reads half as many source frames (pitch down)", "[grain][pitch]")
{
    const int N = 200;
    REQUIRE(srcPosAtPhase(1.0f, N, 0.5f) == Catch::Approx(100.0f));
    REQUIRE(srcPosAtPhase(0.5f, N, 0.5f) == Catch::Approx(50.0f));
}

TEST_CASE("Pitch ratio 1.5 fractional source position is linear", "[grain][pitch]")
{
    const int N = 100;
    REQUIRE(srcPosAtPhase(0.0f,  N, 1.5f) == Catch::Approx(0.0f));
    REQUIRE(srcPosAtPhase(1.0f,  N, 1.5f) == Catch::Approx(150.0f));
    REQUIRE(srcPosAtPhase(0.25f, N, 1.5f) == Catch::Approx(37.5f));
}

// ── GranularEngine threading ──────────────────────────────────────────────────

TEST_CASE("GranularEngine runs without data races", "[engine][threading]")
{
    GranularEngine engine;
    engine.prepare(48000.0, 512);

    juce::AudioBuffer<float> buf(2, 512);

    // Give scheduler time to queue some grains (runs at ~50 Hz = 20 ms/grain)
    juce::Thread::sleep(100); // ~5 scheduler ticks

    for (int i = 0; i < 50; ++i)
    {
        buf.clear();
        engine.processBlock(buf);
    }

    engine.reset();
    // Reaching here under TSAN = no data races detected
    SUCCEED();
}

TEST_CASE("GranularEngine getGrainSnapshots returns plausible data", "[GranularEngine]")
{
    GranularEngine engine;
    engine.prepare(48000.0, 512);
    juce::Thread::sleep(100); // let scheduler fill grains from testSample_

    juce::AudioBuffer<float> out(2, 512);
    engine.processBlock(out); // writes snapshots

    std::array<GranularEngine::GrainSnapshot, GranularEngine::MaxActiveGrains> snaps;
    const int count = engine.getGrainSnapshots(snaps.data(), GranularEngine::MaxActiveGrains);

    REQUIRE(count >= 0);
    REQUIRE(count <= GranularEngine::MaxActiveGrains);
    for (int i = 0; i < count; ++i)
    {
        REQUIRE(snaps[i].srcFraction >= 0.0f);
        REQUIRE(snaps[i].srcFraction <= 1.0f);
        REQUIRE(snaps[i].envelopeAmp >= 0.0f);
        REQUIRE(snaps[i].envelopeAmp <= 1.0f);
    }
    engine.reset();
}

// ── UAT-003 / UAT-017 regressions ─────────────────────────────────────────────

#include "Engine/FeedbackPath.h"
#include "Engine/SpectralProcessor.h"
#include "Engine/MultiSampleBank.h"
#include "Modules/Pattern.h"

TEST_CASE("FeedbackPath and SpectralProcessor honor the +2 guard-sample contract",
          "[engine][guards]")
{
    // renderGrain interpolation may legally read source[len] and source[len+1].
    // Before the fix these two indices were past the end of both arrays (OOB).
    FeedbackPath fb;
    REQUIRE(fb.getReadPointer()[fb.getNumSamples()]     == 0.0f);
    REQUIRE(fb.getReadPointer()[fb.getNumSamples() + 1] == 0.0f);

    SpectralProcessor sp;
    std::array<float, SpectralProcessor::kFFTSize * 2> src{};
    src.fill(0.5f);
    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Freeze, 0.0f);
    REQUIRE(sp.getReadPointer()[sp.getNumSamples()]     == 0.0f);
    REQUIRE(sp.getReadPointer()[sp.getNumSamples() + 1] == 0.0f);

    sp.processSource(src.data(), (int)src.size(), SpectralProcessor::Mode::Blur, 1.0f);
    REQUIRE(sp.getReadPointer()[sp.getNumSamples()]     == 0.0f);
    REQUIRE(sp.getReadPointer()[sp.getNumSamples() + 1] == 0.0f);
}

TEST_CASE("Reversed grains with pitch-up near source end still render",
          "[engine][reverse]")
{
    // position 0.9 + 500 ms grains + pitch +12 st: reversed grains start with an
    // out-of-bounds read position. Before the fix they died at phase 0 (silence);
    // now they skip the OOB prefix and play the in-bounds remainder.
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 512;
    constexpr int    kFrames     = (int)kSampleRate;

    std::atomic<float> grainSize{500.0f}, density{50.0f}, position{0.9f},
                       posJitter{0.0f}, pitchShift{12.0f}, spread{0.5f},
                       masterVol{0.0f}, loop{0.0f};
    std::atomic<float> patEnabled{1.0f}, trigMode{0.0f}, syncDiv{0.0f},
                       euclidPulses{4.0f}, euclidSteps{8.0f}, euclidRot{0.0f},
                       transSens{0.5f}, probability{1.0f}, reverseProb{1.0f},
                       quantScale{0.0f}, spray{0.0f};

    Pattern pattern;
    pattern.setParamPointers(&patEnabled, &trigMode, &syncDiv, &euclidPulses,
                             &euclidSteps, &euclidRot, &transSens, &probability,
                             &reverseProb, &quantScale, &spray);
    pattern.prepare(kSampleRate);

    MultiSampleBank bank;
    auto buf = std::make_unique<juce::AudioBuffer<float>>(1, kFrames + 2);
    buf->clear();
    for (int i = 0; i < kFrames; ++i)
        buf->setSample(0, i, 0.5f);
    bank.loadSlot(0, std::move(buf), kFrames);

    GranularEngine engine;
    engine.setBank(&bank);
    engine.setParamPointers(&grainSize, &density, &position, &posJitter,
                            &pitchShift, &spread, &masterVol, &loop);
    engine.setPatternSource(&pattern);
    engine.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> out(2, kBlockSize);
    out.clear();
    engine.processBlock(out); // trySwapAll installs the slot

    juce::Thread::sleep(400); // let the scheduler queue grains

    float totalEnergy = 0.0f;
    for (int b = 0; b < 100; ++b)
    {
        out.clear();
        engine.processBlock(out);
        for (int s = 0; s < kBlockSize; ++s)
            totalEnergy += std::abs(out.getSample(0, s));
    }
    engine.reset();
    REQUIRE(totalEnergy > 0.0f);
}

// ── UAT-007 / UAT-008 regressions (loop sweep + core mod destinations) ────────

#include "Modulation/ModMatrix.h"

namespace
{
    struct EngineRig
    {
        std::atomic<float> grainSize{100.0f}, density{80.0f}, position{0.0f},
                           posJitter{0.0f}, pitchShift{0.0f}, spread{0.5f},
                           masterVol{0.0f}, loop{0.0f};
        MultiSampleBank bank;
        GranularEngine  engine;

        explicit EngineRig(double sampleRate = 48000.0, int frames = 48000)
        {
            auto buf = std::make_unique<juce::AudioBuffer<float>>(1, frames + 2);
            buf->clear();
            for (int i = 0; i < frames; ++i)
                buf->setSample(0, i, 0.5f);
            bank.loadSlot(0, std::move(buf), frames);

            engine.setBank(&bank);
            engine.setParamPointers(&grainSize, &density, &position, &posJitter,
                                    &pitchShift, &spread, &masterVol, &loop);
            engine.prepare(sampleRate, 512);
        }

        // Runs blocks over ~ms of wall time, returns max grain srcFraction seen.
        float maxSrcFractionOver(int ms, ModMatrix* matrix = nullptr)
        {
            juce::AudioBuffer<float> out(2, 512);
            std::array<GranularEngine::GrainSnapshot, 256> snaps{};
            float maxFrac = 0.0f;
            const auto tEnd = juce::Time::getMillisecondCounter() + (juce::uint32)ms;
            while (juce::Time::getMillisecondCounter() < tEnd)
            {
                if (matrix) matrix->processBlock(512, 120.0);
                out.clear();
                engine.processBlock(out);
                const int n = engine.getGrainSnapshots(snaps.data(), 256);
                for (int i = 0; i < n; ++i)
                    maxFrac = std::max(maxFrac, snaps[(size_t)i].srcFraction);
                juce::Thread::sleep(10);
            }
            return maxFrac;
        }
    };
}

TEST_CASE("Loop toggle sweeps grain position across the source", "[engine][loop]")
{
    // Loop OFF, position 0, jitter 0: grains stay at the start.
    {
        EngineRig rig;
        rig.loop.store(0.0f);
        const float maxFrac = rig.maxSrcFractionOver(700);
        rig.engine.reset();
        REQUIRE(maxFrac < 0.1f);
    }
    // Loop ON: after ~700 ms of a 1 s source, the sweep has moved well past 30%.
    {
        EngineRig rig;
        rig.loop.store(1.0f);
        const float maxFrac = rig.maxSrcFractionOver(700);
        rig.engine.reset();
        REQUIRE(maxFrac > 0.3f);
    }
}

TEST_CASE("ModMatrix kPosition destination moves grain positions", "[engine][moddest]")
{
    // amount 0.5: square pushes position to 0.5 (mid-sample) half the time —
    // grains there live a full 100 ms, long enough for the snapshot to see
    // them (amount 1.0 would clamp position to 1.0 → 1-sample grains that
    // finish before the snapshot pass).
    std::atomic<float> rate{20.0f}, wf{3.0f} /*Square*/, sync{0.0f}, phase{0.0f}, depth{1.0f};
    std::atomic<float> src{1.0f}, dst{(float)ModMatrix::kPosition}, amt{0.5f};
    Lfo lfo;
    lfo.setParamPointers(&rate, &wf, &sync, &phase, &depth);
    lfo.prepare(48000.0);
    ModMatrix matrix;
    matrix.setLfos(&lfo, nullptr);
    matrix.setSlotParams(0, &src, &dst, &amt);
    matrix.prepare(48000.0);

    EngineRig rig;
    rig.engine.reset();                      // stop scheduler to rewire safely
    rig.engine.setModMatrixSource(&matrix);
    rig.engine.prepare(48000.0, 512);

    // Square LFO drives position 0 -> 0.5 half the time; without the mod the
    // engine (position 0, jitter 0) never leaves the start of the source.
    const float maxFrac = rig.maxSrcFractionOver(700, &matrix);
    rig.engine.reset();
    REQUIRE(maxFrac > 0.4f);
}

// ── 08-08 R-01: sample-clock scheduling ───────────────────────────────────────

namespace
{
    struct ClockRig
    {
        std::atomic<float> grainSize{50.0f}, density{50.0f}, position{0.2f},
                           posJitter{0.3f}, pitchShift{0.0f}, spread{0.5f},
                           masterVol{0.0f}, loop{0.0f};
        std::atomic<float> patEnabled{0.0f}, trigMode{0.0f}, syncDiv{2.0f} /*1/16*/,
                           euclidPulses{4.0f}, euclidSteps{8.0f}, euclidRot{0.0f},
                           transSens{0.5f}, probability{1.0f}, reverseProb{0.0f},
                           quantScale{0.0f}, spray{0.0f};
        Pattern        pattern;
        GranularEngine engine;

        ClockRig()
        {
            pattern.setParamPointers(&patEnabled, &trigMode, &syncDiv, &euclidPulses,
                                     &euclidSteps, &euclidRot, &transSens, &probability,
                                     &reverseProb, &quantScale, &spray);
            pattern.prepare(48000.0);
            engine.setParamPointers(&grainSize, &density, &position, &posJitter,
                                    &pitchShift, &spread, &masterVol, &loop);
            engine.setPatternSource(&pattern);
            engine.prepare(48000.0, 512);
            engine.setRandomSeed(1234);
        }

        // Renders `totalSamples` back-to-back (offline: no wall-clock waits)
        // in blocks of `block`, appending the left channel to `out`.
        void render(int totalSamples, int block, std::vector<float>* out = nullptr,
                    bool playing = false, double bpm = 120.0)
        {
            juce::AudioBuffer<float> buf(2, block);
            const double spb = 48000.0 * 60.0 / bpm;
            for (int done = 0; done < totalSamples; done += block)
            {
                const int n = std::min(block, totalSamples - done);
                juce::AudioBuffer<float> sub(buf.getArrayOfWritePointers(), 2, 0, n);
                engine.setTransport(bpm, playing, playing, (double)done / spb);
                engine.processBlock(sub);
                if (out)
                    out->insert(out->end(), sub.getReadPointer(0), sub.getReadPointer(0) + n);
            }
        }
    };
}

TEST_CASE("Offline back-to-back rendering spawns density x duration grains",
          "[engine][clock]")
{
    // Before R-01 the scheduler slept on the wall clock, so a render with no
    // real-time waits produced ~0 grains. 10 s at 50 g/s must give ~500.
    for (int block : { 64, 512, 4096 })
    {
        ClockRig rig;
        rig.render(48000 * 10, block);
        const auto n = rig.engine.getSpawnCount();
        INFO("block " << block << " spawned " << n);
        REQUIRE(n >= 475);
        REQUIRE(n <= 525);
    }
}

TEST_CASE("Grain timing is identical for any host block size", "[engine][clock]")
{
    // Same seed, same parameters: output must be bit-identical whether the
    // host calls us with 64- or 4096-sample blocks (onsets are sample-accurate,
    // not snapped to block boundaries).
    std::vector<float> a, b;
    { ClockRig rig; rig.render(48000 * 2, 64,   &a); }
    { ClockRig rig; rig.render(48000 * 2, 4096, &b); }
    REQUIRE(a.size() == b.size());
    double maxDiff = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
        maxDiff = std::max(maxDiff, (double)std::abs(a[i] - b[i]));
    REQUIRE(maxDiff < 1.0e-6);
}

TEST_CASE("Euclidean steps lock to the host PPQ grid", "[engine][clock][pattern]")
{
    // 1/16 steps, E(4,8), 16 beats at 120 BPM while playing = 64 steps,
    // 32 hits — exactly, regardless of block size.
    for (int block : { 100, 512, 2048 })
    {
        ClockRig rig;
        rig.patEnabled.store(1.0f);
        rig.trigMode.store(2.0f); // Euclidean
        const int total = (int)(48000.0 * 60.0 / 120.0 * 16.0); // 16 beats
        rig.render(total, block, nullptr, /*playing*/ true);
        INFO("block " << block);
        REQUIRE(rig.engine.getSpawnCount() == 32);
    }
}

TEST_CASE("Stereo spread: 0 is mono, default 0.5 is stereo", "[engine][spread]")
{
    auto sideEnergy = [](float spread)
    {
        ClockRig rig;
        rig.spread.store(spread);
        juce::AudioBuffer<float> buf(2, 512);
        double side = 0.0, mid = 0.0;
        for (int b = 0; b < 200; ++b)
        {
            rig.engine.processBlock(buf);
            for (int i = 0; i < 512; ++i)
            {
                const double l = buf.getSample(0, i), r = buf.getSample(1, i);
                side += (l - r) * (l - r);
                mid  += (l + r) * (l + r);
            }
        }
        return side / std::max(mid, 1e-12);
    };
    REQUIRE(sideEnergy(0.0f) < 1.0e-9);  // R-02: spread 0 = centre
    REQUIRE(sideEnergy(0.5f) > 0.01);    // default is NOT mono
    REQUIRE(sideEnergy(1.0f) > sideEnergy(0.5f));
}

// ── 08-08 T2: grain geometry, precision, envelope ─────────────────────────────

TEST_CASE("Gaussian envelope reaches zero at both edges", "[envelope]")
{
    REQUIRE(applyEnvelope(EnvelopeShape::Gaussian, 0.0f) == Catch::Approx(0.0f).margin(1e-6));
    REQUIRE(applyEnvelope(EnvelopeShape::Gaussian, 1.0f) == Catch::Approx(0.0f).margin(1e-6));
    REQUIRE(applyEnvelope(EnvelopeShape::Gaussian, 0.5f) == Catch::Approx(1.0f).margin(1e-5));
}

TEST_CASE("fitGrainToSource keeps every read span inside the source", "[engine][bounds]")
{
    // Exhaustive sweep: position x pitch x length x reverse — the read span
    // [startPos, startPos + len*pitch] must lie in [0, sourceLen-1] and the
    // grain must keep its full envelope (no truncation, R-03).
    static float src[10000 + 2]{};
    for (float pos : { 0.0f, 0.5f, 0.9f, 0.99f, 1.0f })
        for (float st : { -24.0f, -7.0f, 0.0f, 7.0f, 12.0f, 24.0f })
            for (int len : { 1, 100, 2000, 9999, 50000 })
                for (bool rev : { false, true })
                {
                    Grain g{};
                    g.source = src;
                    g.sourceLen = 10000;
                    g.lengthSamples = len;
                    g.pitchRatio = std::pow(2.0f, st / 12.0f);
                    g.reversed = rev;
                    g.startPos = (int)(pos * 9999.0f);
                    fitGrainToSource(g);
                    INFO("pos " << pos << " st " << st << " len " << len);
                    REQUIRE(g.startPos >= 0);
                    REQUIRE(g.lengthSamples >= 1);
                    REQUIRE((double)g.startPos + (double)g.lengthSamples * g.pitchRatio
                            <= 9999.0 + 1e-6);
                    if ((double)len * g.pitchRatio <= 9999.0)
                        REQUIRE(g.lengthSamples == len); // never shortened when it fits
                }
}

TEST_CASE("Grains near the source end play a complete envelope", "[engine][bounds]")
{
    // Position 0.96, +12 st, 100 ms grains on a 1 s source: before R-03 the
    // read ran off the end at phase 0.5 and the grain was cut at peak level.
    // Now every grain must decay to ~0 at its final sample.
    std::atomic<float> grainSize{100.0f}, density{20.0f}, position{0.96f},
                       posJitter{0.0f}, pitchShift{12.0f}, spread{0.0f},
                       masterVol{0.0f}, loop{0.0f};
    MultiSampleBank bank;
    auto buf = std::make_unique<juce::AudioBuffer<float>>(1, 48000 + 2);
    buf->clear();
    for (int i = 0; i < 48000; ++i) buf->setSample(0, i, 1.0f); // DC: output == envelope
    bank.loadSlot(0, std::move(buf), 48000);

    GranularEngine engine;
    engine.setBank(&bank);
    engine.setParamPointers(&grainSize, &density, &position, &posJitter,
                            &pitchShift, &spread, &masterVol, &loop);
    engine.prepare(48000.0, 512);
    engine.setRandomSeed(7);

    juce::AudioBuffer<float> out(2, 64);
    float prev = 0.0f, maxJump = 0.0f;
    for (int b = 0; b < 48000 / 64; ++b)
    {
        engine.processBlock(out);
        for (int i = 0; i < 64; ++i)
        {
            const float v = out.getSample(0, i);
            maxJump = std::max(maxJump, std::abs(v - prev));
            prev = v;
        }
    }
    // Hann grains of 4800 samples: max slope ≈ pi/4800 ≈ 6.5e-4 per sample
    // per grain. A truncation would jump by up to ~0.5.
    REQUIRE(maxJump < 0.01f);
}

TEST_CASE("Long sources keep sub-sample read precision", "[engine][precision]")
{
    // A ramp source (value == index) over 30 M frames: at position 0.9 a
    // float read position quantises to 2 samples (R-05). With the double
    // read position, consecutive output samples of a pitch 0.5 grain must
    // step by exactly 0.5 (times the envelope).
    constexpr int kLen = 30'000'000;
    std::vector<float> ramp((size_t)kLen + 2);
    // Ramp relative to the read region keeps float values exact.
    const int base = (int)(0.9 * (kLen - 1));
    for (int i = 0; i < kLen + 2; ++i)
        ramp[(size_t)i] = (float)(i - base);

    Grain g{};
    g.source = ramp.data();
    g.sourceLen = kLen;
    g.startPos = base;
    g.lengthSamples = 1000;
    g.pitchRatio = 0.5f;
    g.shape = EnvelopeShape::Rectangle;
    g.pan = -1.0f; // all to L, lGain = 1.0
    fitGrainToSource(g);
    REQUIRE(g.startPos == base);

    std::vector<float> L(1000, 0.0f), R(1000, 0.0f);
    REQUIRE(GranularEngine::renderGrain(&g, L.data(), R.data(), 1000));
    for (int e = 0; e < 1000; ++e)
        REQUIRE(L[(size_t)e] == Catch::Approx((double)e * 0.5).margin(1e-3));
}

// ── 08-08 T3 / T5: level safety + feedback ring positioning ──────────────────

namespace
{
    struct FbRig
    {
        std::atomic<float> grainSize{50.0f}, density{1.0f}, position{0.0f},
                           posJitter{0.0f}, pitchShift{0.0f}, spread{0.0f},
                           masterVol{0.0f}, loop{0.0f};
        std::atomic<float> fbOn{1.0f}, fbGain{0.95f}, fbDamp{0.0f};
        std::atomic<float> shape{4.0f}; // Rectangle
        std::atomic<float> subDepth{0.0f}, dist{0.0f};
        FeedbackPath   fb;
        GranularEngine engine;

        FbRig()
        {
            fb.prepare(48000.0);
            engine.setParamPointers(&grainSize, &density, &position, &posJitter,
                                    &pitchShift, &spread, &masterVol, &loop);
            engine.setAdvancedParamPointers(&subDepth, &dist);
            engine.setEnvelopeShapeParam(&shape);
            engine.setFeedbackSource(&fb);
            engine.setFeedbackParamPointers(&fbOn, &fbGain, &fbDamp);
            engine.prepare(48000.0, 512);
            engine.setRandomSeed(3);
        }
    };
}

TEST_CASE("Feedback grains read only recent audio behind the write head",
          "[engine][feedback]")
{
    // Ring input is 0.2 for 4.5 s, then 0.8. Position 0 = most recent audio:
    // once the new content is older than a grain span, every sounding sample
    // must come from the 0.8 region. A grain reading across the write head
    // (R-12) would pull in the 4-second-old 0.2 audio.
    FbRig rig;
    const float hi = std::tanh(0.8f * 0.95f) * 0.5f; // pan 0 → lGain 0.5, gain 1
    std::vector<float> in(512);
    juce::AudioBuffer<float> out(2, 512);
    int badSamples = 0, soundingSamples = 0;
    for (int b = 0; b < (48000 * 8) / 512; ++b)
    {
        const double t = (double)b * 512.0 / 48000.0;
        std::fill(in.begin(), in.end(), t < 4.5 ? 0.2f : 0.8f);
        rig.engine.processBlock(out);
        rig.fb.process(in.data(), in.data(), 512, 0.95f, 0.0f);
        if (t > 4.7)
            for (int i = 0; i < 512; ++i)
            {
                const float v = out.getSample(0, i);
                if (std::abs(v) < 1e-6f) continue;
                ++soundingSamples;
                if (std::abs(v - hi) > 1e-3f) ++badSamples;
            }
    }
    REQUIRE(soundingSamples > 1000);
    REQUIRE(badSamples == 0);
}

TEST_CASE("Feedback loop at max gain and density stays finite and bounded",
          "[engine][feedback][level]")
{
    // Closed loop: engine output → ring → feedback grains. Before R-11 the
    // loop gain exceeded 1 at high overlap and ran to inf/NaN.
    FbRig rig;
    rig.density.store(100.0f);
    rig.grainSize.store(100.0f);
    rig.shape.store(0.0f); // Hann
    rig.spread.store(0.5f);
    juce::AudioBuffer<float> out(2, 512);
    // Seed the ring with a burst.
    std::vector<float> burst(512, 0.9f);
    rig.fb.process(burst.data(), burst.data(), 512, 0.95f, 0.0f);
    float peak = 0.0f;
    int nonFinite = 0;
    for (int b = 0; b < (48000 * 60) / 512; ++b)
    {
        rig.engine.processBlock(out);
        rig.fb.process(out.getReadPointer(0), out.getReadPointer(1), 512, 0.95f, 0.0f);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i)
            {
                const float v = out.getSample(c, i);
                if (!std::isfinite(v)) ++nonFinite;
                else peak = std::max(peak, std::abs(v));
            }
    }
    REQUIRE(nonFinite == 0);
    REQUIRE(peak < 4.0f);
}

TEST_CASE("Dense overlapping grains stay near unity level", "[engine][level]")
{
    // Density 100, size 500 ms (≈50 overlapping grains), jitter 0, sub-grain
    // depth 2: before R-04 the sum reached ~+28 dBFS.
    std::atomic<float> grainSize{500.0f}, density{100.0f}, position{0.3f},
                       posJitter{0.0f}, pitchShift{0.0f}, spread{0.5f},
                       masterVol{0.0f}, loop{0.0f}, subDepth{2.0f}, dist{0.0f};
    GranularEngine engine; // 440 Hz test sine, amplitude 1
    engine.setParamPointers(&grainSize, &density, &position, &posJitter,
                            &pitchShift, &spread, &masterVol, &loop);
    engine.setAdvancedParamPointers(&subDepth, &dist);
    engine.prepare(48000.0, 512);
    engine.setRandomSeed(11);
    juce::AudioBuffer<float> out(2, 512);
    float peak = 0.0f;
    for (int b = 0; b < (48000 * 5) / 512; ++b)
    {
        engine.processBlock(out);
        peak = std::max(peak, out.getMagnitude(0, 512));
    }
    INFO("peak " << peak);
    REQUIRE(peak <= 2.0f); // ≤ +6 dBFS
    REQUIRE(peak > 0.1f);
}

TEST_CASE("Master volume changes are ramped, not stepped", "[engine][level]")
{
    std::atomic<float> masterVol{-60.0f};
    std::atomic<float> d{0.0f};
    GranularEngine engine;
    engine.setParamPointers(&d, &d, &d, &d, &d, &d, &masterVol, &d);
    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();
    for (int i = 0; i < 512; ++i) { buf.setSample(0, i, 1.0f); buf.setSample(1, i, 1.0f); }
    engine.applyMasterGain(buf); // establishes -60 dB
    for (int i = 0; i < 512; ++i) { buf.setSample(0, i, 1.0f); buf.setSample(1, i, 1.0f); }
    masterVol.store(6.0f);
    engine.applyMasterGain(buf); // -60 → +6 dB over this block
    float maxStep = 0.0f;
    for (int i = 1; i < 512; ++i)
        maxStep = std::max(maxStep, std::abs(buf.getSample(0, i) - buf.getSample(0, i - 1)));
    REQUIRE(buf.getSample(0, 0) < 0.01f);
    REQUIRE(buf.getSample(0, 511) > 1.9f);
    REQUIRE(maxStep < 0.01f); // ~2.0 / 512 per sample
}

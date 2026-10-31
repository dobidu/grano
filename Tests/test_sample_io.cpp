#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Engine/SampleBuffer.h"
#include "Engine/MultiSampleBank.h"
#include "Engine/GranularEngine.h"
#include "PluginProcessor.h"

// ── Helpers ──────────────────────────────────────────────────────────────────

static std::unique_ptr<juce::AudioBuffer<float>> makeBuffer(int numFrames, float fillValue = 0.5f)
{
    // +2 guard samples as required by the SampleBuffer contract.
    auto buf = std::make_unique<juce::AudioBuffer<float>>(1, numFrames + 2);
    buf->clear();
    for (int i = 0; i < numFrames; ++i)
        buf->setSample(0, i, fillValue);
    return buf;
}

// ── SampleBuffer — basic lifecycle ───────────────────────────────────────────

TEST_CASE("SampleBuffer starts empty", "[SampleBuffer]")
{
    SampleBuffer sb;
    REQUIRE(sb.getReadPointer() == nullptr);
    REQUIRE(sb.getNumSamples() == 0);
}

TEST_CASE("SampleBuffer trySwap returns false when nothing pending", "[SampleBuffer]")
{
    SampleBuffer sb;
    REQUIRE_FALSE(sb.trySwap());
}

TEST_CASE("SampleBuffer setPending then trySwap installs buffer", "[SampleBuffer]")
{
    SampleBuffer sb;
    constexpr int N = 1000;
    sb.setPending(makeBuffer(N), N);

    REQUIRE(sb.trySwap());
    REQUIRE(sb.getReadPointer() != nullptr);
    REQUIRE(sb.getNumSamples() == N);
}

TEST_CASE("SampleBuffer getNumSamples reflects only playable frames", "[SampleBuffer]")
{
    SampleBuffer sb;
    constexpr int audioFrames = 44100;
    sb.setPending(makeBuffer(audioFrames), audioFrames);
    (void) sb.trySwap();
    REQUIRE(sb.getNumSamples() == audioFrames);
}

TEST_CASE("SampleBuffer read pointer points to channel-0 data", "[SampleBuffer]")
{
    SampleBuffer sb;
    constexpr int N = 512;
    auto buf = makeBuffer(N, 0.75f);
    const float* rawPtr = buf->getReadPointer(0);
    sb.setPending(std::move(buf), N);
    (void) sb.trySwap();
    REQUIRE(sb.getReadPointer() == rawPtr);
    REQUIRE(sb.getReadPointer()[0] == Catch::Approx(0.75f));
}

// ── SampleBuffer — second swap retires the first buffer ───────────────────────

TEST_CASE("SampleBuffer second setPending+trySwap replaces first buffer", "[SampleBuffer]")
{
    SampleBuffer sb;
    sb.setPending(makeBuffer(100, 0.1f), 100);
    (void) sb.trySwap();

    sb.setPending(makeBuffer(200, 0.9f), 200);
    REQUIRE(sb.trySwap()); // old buffer should be retired, new installed

    REQUIRE(sb.getNumSamples() == 200);
    REQUIRE(sb.getReadPointer()[0] == Catch::Approx(0.9f));
}

TEST_CASE("SampleBuffer processRetired drains without crash", "[SampleBuffer]")
{
    SampleBuffer sb;
    for (int i = 0; i < 3; ++i)
    {
        sb.setPending(makeBuffer(100), 100);
        (void) sb.trySwap();
    }
    // Should not crash or leak (JUCE leak detector verifies the latter).
    sb.processRetired();
}

// ── SampleBuffer — unclaimed pending is cleaned up on next setPending ────────

TEST_CASE("SampleBuffer cleans up unclaimed pending on next setPending", "[SampleBuffer]")
{
    SampleBuffer sb;
    sb.setPending(makeBuffer(100), 100); // pending, never claimed
    sb.setPending(makeBuffer(200), 200); // replaces the first (deletes it)
    // If we call trySwap now we get the second one.
    REQUIRE(sb.trySwap());
    REQUIRE(sb.getNumSamples() == 200);
}

TEST_CASE("SampleBuffer trySwap is idempotent when no new pending", "[SampleBuffer]")
{
    SampleBuffer sb;
    sb.setPending(makeBuffer(100), 100);
    (void) sb.trySwap();

    REQUIRE_FALSE(sb.trySwap()); // no new pending — should return false
    REQUIRE(sb.getNumSamples() == 100); // still has the old buffer
}

// ── GranularEngine — SampleBuffer integration ────────────────────────────────

TEST_CASE("GranularEngine uses SampleBuffer source when loaded", "[GranularEngine][SampleBuffer]")
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 512;
    constexpr int    kFrames     = (int)kSampleRate; // 1 second

    MultiSampleBank bank;
    GranularEngine engine;
    engine.setBank(&bank);
    engine.prepare(kSampleRate, kBlockSize);

    // Load slot 0, then trigger one processBlock to call trySwapAll().
    bank.loadSlot(0, makeBuffer(kFrames, 0.5f), kFrames);

    juce::AudioBuffer<float> out(2, kBlockSize);
    out.clear();
    engine.processBlock(out); // triggers trySwap(); currentReadPtr_ now set

    // Give scheduler time to queue multiple grains (Uniform [50,150]ms interval,
    // 400ms guarantees 2-4 grains in FIFO before we start measuring).
    juce::Thread::sleep(400);

    float totalEnergy = 0.0f;
    for (int b = 0; b < 50; ++b)
    {
        out.clear();
        engine.processBlock(out);
        for (int s = 0; s < kBlockSize; ++s)
            totalEnergy += std::abs(out.getSample(0, s));
    }

    engine.reset();
    REQUIRE(totalEnergy > 0.0f); // non-zero output means SampleBuffer was used
}

TEST_CASE("GranularEngine falls back to sine tone when SampleBuffer is empty", "[GranularEngine][SampleBuffer]")
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 512;

    MultiSampleBank bank;
    GranularEngine engine;
    engine.setBank(&bank); // no slots loaded yet — falls back to sine
    engine.prepare(kSampleRate, kBlockSize);

    juce::Thread::sleep(400);

    juce::AudioBuffer<float> out(2, kBlockSize);
    float totalEnergy = 0.0f;
    for (int b = 0; b < 50; ++b)
    {
        out.clear();
        engine.processBlock(out);
        for (int s = 0; s < kBlockSize; ++s)
            totalEnergy += std::abs(out.getSample(0, s));
    }

    engine.reset();
    REQUIRE(totalEnergy > 0.0f); // sine fallback produces audio
}

TEST_CASE("GranularEngine handles null SampleBuffer gracefully", "[GranularEngine][SampleBuffer]")
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 512;

    GranularEngine engine; // no setSource — sampleSource_ stays nullptr
    engine.prepare(kSampleRate, kBlockSize);

    juce::Thread::sleep(100);

    juce::AudioBuffer<float> out(2, kBlockSize);
    // Should not crash.
    for (int b = 0; b < 10; ++b)
    {
        out.clear();
        engine.processBlock(out);
    }

    engine.reset();
    SUCCEED("No crash with null SampleBuffer");
}

// ── State persistence — sample paths (UAT-002 regression) ────────────────────

static juce::File writeTestWav(const juce::String& name, int numFrames)
{
    auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile(name);
    file.deleteFile();

    juce::AudioBuffer<float> buf(1, numFrames);
    for (int i = 0; i < numFrames; ++i)
        buf.setSample(0, i, std::sin(0.05f * (float)i) * 0.5f);

    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(
        new juce::FileOutputStream(file), 44100.0, 1, 16, {}, 0));
    REQUIRE(writer != nullptr);
    writer->writeFromAudioSampleBuffer(buf, 0, numFrames);
    writer.reset(); // flush
    return file;
}

TEST_CASE("Plugin state round-trip restores loaded samples", "[State][MultiSampleBank]")
{
    const auto f0 = writeTestWav("grano_uat002_slot0.wav", 1000);
    const auto f2 = writeTestWav("grano_uat002_slot2.wav", 2000);

    juce::MemoryBlock state;
    {
        GranoAudioProcessor proc;
        proc.loadSampleFile(f0, 0);
        proc.loadSampleFile(f2, 2);
        REQUIRE(proc.getLastLoadError().isEmpty());
        proc.getStateInformation(state);
    }

    GranoAudioProcessor restored;
    restored.setStateInformation(state.getData(), (int)state.getSize());
    (void)restored.getSampleBank().trySwapAll(); // audio-thread step

    REQUIRE(restored.getSampleBank().getNumSamples(0) == 1000);
    REQUIRE(restored.getSampleBank().getNumSamples(2) == 2000);
    REQUIRE(restored.getSampleBank().getReadPointer(0) != nullptr);
    REQUIRE(restored.getSampleBank().getReadPointer(2) != nullptr);
    REQUIRE(restored.getSampleBank().getReadPointer(1) == nullptr);
    REQUIRE(restored.getSampleBank().getReadPointer(3) == nullptr);

    f0.deleteFile();
    f2.deleteFile();
}

TEST_CASE("Plugin state restore degrades gracefully on missing sample file", "[State][MultiSampleBank]")
{
    const auto f0 = writeTestWav("grano_uat002b_slot0.wav", 500);
    const auto f1 = writeTestWav("grano_uat002b_slot1.wav", 600);

    juce::MemoryBlock state;
    {
        GranoAudioProcessor proc;
        proc.loadSampleFile(f0, 0);
        proc.loadSampleFile(f1, 1);
        proc.getStateInformation(state);
    }

    f1.deleteFile(); // simulate moved/deleted file

    GranoAudioProcessor restored;
    restored.setStateInformation(state.getData(), (int)state.getSize());
    (void)restored.getSampleBank().trySwapAll();

    // Present file loads; missing file reports an error without aborting restore.
    REQUIRE(restored.getSampleBank().getNumSamples(0) == 500);
    REQUIRE(restored.getSampleBank().getReadPointer(1) == nullptr);
    REQUIRE(restored.getLastLoadError().contains("not found"));

    f0.deleteFile();
}

TEST_CASE("Enabling spectral at runtime reprocesses source and updates latency",
          "[State][Spectral]")
{
    // UAT-006: before the fix, enabling spectral after load made the engine
    // granulate a zeroed buffer (silence) and never re-reported latency.
    const auto f0 = writeTestWav("grano_uat006.wav", 8192);

    GranoAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    proc.loadSampleFile(f0, 0);
    REQUIRE(proc.getLatencySamples() == 0);

    // Flip spectralEnabled the way a host would; listener flags the change.
    auto* param = proc.getAPVTS().getParameter(ParamIDs::spectralEnabled);
    REQUIRE(param != nullptr);
    param->setValueNotifyingHost(1.0f);
    proc.applyPendingSpectralUpdate(); // timer surrogate (headless: no MessageManager)

    // R-10: spectral is offline on the source — no audio-path latency.
    REQUIRE(proc.getLatencySamples() == 0);

    // Engine now sources from the spectral buffer — output must not be silent.
    juce::AudioBuffer<float> buf(2, 512);
    juce::MidiBuffer midi;
    buf.clear();
    proc.processBlock(buf, midi); // trySwapAll makes the loaded slot visible
    proc.applyPendingSpectralUpdate(); // retry now that the source is swapped in
    juce::Thread::sleep(400);

    float energy = 0.0f;
    for (int b = 0; b < 100; ++b)
    {
        buf.clear();
        proc.processBlock(buf, midi);
        for (int s = 0; s < 512; ++s)
            energy += std::abs(buf.getSample(0, s));
    }
    REQUIRE(energy > 0.0f);

    // Disabling clears the reported latency again.
    param->setValueNotifyingHost(0.0f);
    proc.applyPendingSpectralUpdate();
    REQUIRE(proc.getLatencySamples() == 0);

    proc.releaseResources();
    f0.deleteFile();
}

// ── 08-08: SampleBuffer record swap + spectral reload ────────────────────────

TEST_CASE("SampleBuffer keeps each buffer paired with its own length", "[SampleBuffer]")
{
    // R-06: the length used to be stored separately before the pointer
    // exchange, so a second load before the swap could pair buffer A with
    // len(B). Now length + generation travel with the buffer.
    SampleBuffer sb;
    const auto g1 = sb.setPending(makeBuffer(100), 100);
    const auto g2 = sb.setPending(makeBuffer(5000), 5000);
    REQUIRE(g2 > g1);
    REQUIRE(sb.trySwap());
    REQUIRE(sb.getNumSamples() == 5000);
    REQUIRE(sb.getCurrentGeneration() == g2);
    // Guard samples present → the buffer really is 5000 + 2 long.
    REQUIRE(sb.getReadPointer()[5001] == 0.0f);
}

TEST_CASE("SampleBuffer defers swaps instead of freeing on the audio thread", "[SampleBuffer]")
{
    // R-21: with the retire queue full the old code deleted the current
    // buffer on the audio thread. Now the swap waits for processRetired().
    SampleBuffer sb;
    int swaps = 0;
    for (int i = 0; i < 40; ++i)
    {
        sb.setPending(makeBuffer(100 + i), 100 + i);
        if (sb.trySwap()) ++swaps;
    }
    REQUIRE(swaps < 40);          // some swaps deferred (queue full)
    REQUIRE(sb.getNumSamples() > 0);
    sb.processRetired();          // message thread drains the queue
    REQUIRE(sb.trySwap());        // deferred load now goes live
    REQUIRE(sb.getNumSamples() == 139);
}

TEST_CASE("Loading a new sample with spectral on derives the spectrum from it",
          "[State][Spectral]")
{
    // R-09: loadSampleFile used to run the FFT immediately — on the OLD
    // slot-0 buffer, because the new one is only pending until the audio
    // thread swaps it in.
    const auto fA = writeTestWav("grano_r09_a.wav", 8192);
    GranoAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    auto* param = proc.getAPVTS().getParameter(ParamIDs::spectralEnabled);
    param->setValueNotifyingHost(1.0f);

    juce::AudioBuffer<float> buf(2, 512);
    juce::MidiBuffer midi;
    proc.loadSampleFile(fA, 0);
    proc.processBlock(buf, midi);
    proc.applyPendingSpectralUpdate();

    // Sample B: different content (noise) in the same slot.
    const auto fB = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("grano_r09_b.wav");
    {
        fB.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(
            new juce::FileOutputStream(fB), 48000.0, 1, 16, {}, 0));
        juce::AudioBuffer<float> noise(1, 8192);
        juce::Random r(5);
        for (int i = 0; i < 8192; ++i) noise.setSample(0, i, r.nextFloat() - 0.5f);
        w->writeFromAudioSampleBuffer(noise, 0, 8192);
    }
    proc.loadSampleFile(fB, 0);
    proc.applyPendingSpectralUpdate(); // B still pending → must NOT process A again
    proc.processBlock(buf, midi);      // audio thread swaps B in
    proc.applyPendingSpectralUpdate(); // now derived from B

    SpectralProcessor expected;
    expected.processSource(proc.getSampleBank().getReadPointer(0),
                           proc.getSampleBank().getNumSamples(0),
                           SpectralProcessor::Mode::Freeze, 0.0f);
    const float* got = proc.getSpectralProcessor().getReadPointer();
    float maxDiff = 0.0f;
    for (int i = 0; i < SpectralProcessor::kFFTSize; ++i)
        maxDiff = std::max(maxDiff, std::abs(got[i] - expected.getReadPointer()[i]));
    REQUIRE(maxDiff < 1e-5f);

    proc.releaseResources();
    fA.deleteFile();
    fB.deleteFile();
}

TEST_CASE("Restoring a state without samples/snapshots clears stale ones", "[State]")
{
    // R-23: deserialise only overwrote slots present in the data, so the old
    // instance's samples kept playing and its snapshots survived a preset load.
    GranoAudioProcessor fresh;
    juce::MemoryBlock emptyState;
    fresh.getStateInformation(emptyState);

    const auto f1 = writeTestWav("grano_r23.wav", 700);
    GranoAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    proc.loadSampleFile(f1, 1);
    proc.getSnapshots().save(2, proc.getAPVTS().copyState());
    juce::AudioBuffer<float> buf(2, 512);
    juce::MidiBuffer midi;
    proc.processBlock(buf, midi);
    REQUIRE(proc.getSampleBank().getNumSamples(1) == 700);

    proc.setStateInformation(emptyState.getData(), (int)emptyState.getSize());
    proc.processBlock(buf, midi); // audio thread swaps the empty record in

    REQUIRE(proc.getSampleBank().getNumSamples(1) == 0);
    REQUIRE(proc.getSampleBank().getReadPointer(1) == nullptr);
    REQUIRE(proc.getSlotFile(1) == juce::File());
    REQUIRE_FALSE(proc.getSnapshots().isOccupied(2));

    proc.releaseResources();
    f1.deleteFile();
}

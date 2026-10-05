#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Media management/export/iexport_service.h"
#include "Media management/registry/imedia_registry.h"
#include "Media management/codecs/sndfile_reader.h"
#include "Media management/codecs/sndfile_writer.h"
#include "Core infrastructure/memory/istring_registry.h"
#include "Core audio engine/scheduler/idsp_kernel.h"
#include <thread>
#include <chrono>
#include <vector>
#include <cmath>
#include <filesystem>

using namespace MediaManagement;

// Synthesize 50Hz kick sine wave
static void kickSimulatorProcessor(NodeID nodeId, float* const* inputs, float* const* outputs,
                                   uint32_t numChannels, uint32_t numSamples,
                                   const EventData* events, uint32_t numEvents,
                                   EventData* outEvents, uint32_t* outEventCount,
                                   const ProcessContext* context) {
    (void)nodeId; (void)inputs; (void)events; (void)numEvents; (void)outEvents; (void)outEventCount;
    uint64_t startSample = context ? context->transport.positionSample : 0;
    float sampleRate = (context && context->sampleRate > 0.0f) ? context->sampleRate : 44100.0f;
    constexpr float freq = 50.0f; // 50 Hz sub-bass kick

    for (uint32_t s = 0; s < numSamples; ++s) {
        uint64_t globalSample = startSample + s;
        float t = static_cast<float>(globalSample) / sampleRate;
        float val = 0.8f * std::sin(2.0f * 3.14159265f * freq * t);
        for (uint32_t c = 0; c < numChannels; ++c) {
            if (outputs && outputs[c]) {
                outputs[c][s] = val;
            }
        }
    }
}

TEST_CASE("SndFileWriter Over-0dBFS Peak Handling & Anti-Foldback", "[MediaManagement][Codecs][Fidelity]") {
    const std::string testWav = "test_over_zero_db.wav";
    const int sampleRate = 44100;
    const int numChannels = 1;
    const int numFrames = 1000;

    // Create 16-bit PCM file
    {
        SndFileWriter writer(testWav, sampleRate, numChannels, SF_FORMAT_WAV | SF_FORMAT_PCM_16);
        REQUIRE(writer.isValid());

        std::vector<float> inputSamples(numFrames);
        for (int i = 0; i < numFrames; ++i) {
            // Signal exceeding 0 dBFS up to +3.5 dBFS (1.5f)
            float t = static_cast<float>(i) / sampleRate;
            inputSamples[i] = 1.5f * std::sin(2.0f * 3.14159265f * 100.0f * t);
        }

        uint32_t written = writer.writeFrames(inputSamples.data(), numFrames);
        REQUIRE(written == numFrames);
    }

    // Read back and verify there is NO foldback integer wrap-around
    {
        SndFileReader reader(testWav);
        REQUIRE(reader.isValid());
        REQUIRE(reader.getTotalFrames() == numFrames);

        std::vector<float> readSamples(numFrames);
        uint32_t read = reader.readFrames(readSamples.data(), numFrames);
        REQUIRE(read == numFrames);

        for (int i = 0; i < numFrames; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            float rawSine = std::sin(2.0f * 3.14159265f * 100.0f * t);

            // During positive crest where rawSine > 0.8:
            // Input was 1.5 * rawSine > 1.0 (over 0 dBFS).
            // Without clipping, integer overflow wraps +1.5 to negative (-0.8).
            // With proper clipping, readSamples[i] MUST stay positive (~1.0f).
            if (rawSine > 0.8f) {
                CHECK(readSamples[i] > 0.0f);
                CHECK(readSamples[i] <= 1.0001f);
            } else if (rawSine < -0.8f) {
                CHECK(readSamples[i] < 0.0f);
                CHECK(readSamples[i] >= -1.0001f);
            }
        }
    }

    std::filesystem::remove(testWav);
}

TEST_CASE("ExportService End-to-End Kick Fidelity Verification", "[MediaManagement][Export][Fidelity]") {
    auto registry = IMediaRegistry::create();
    auto strings = Layer2::IStringRegistry::create();
    auto kernel = Layer3::IDSPKernel::create();

    // Register 50Hz kick processor
    kernel->registerProcessor(0, kickSimulatorProcessor);

    auto exportService = IExportService::create(registry.get(), strings.get(), kernel.get());
    REQUIRE(exportService != nullptr);

    const std::string outPath = "test_kick_fidelity_render.wav";
    uint32_t outPathId = strings->registerString(outPath);

    ExportConfig config{};
    config.outputPathId = outPathId;
    config.format = ExportFormat::WAV;
    config.bitDepth = ExportBitDepth::BIT_24;
    config.sampleRate = 44100;
    config.numChannels = 2;
    config.startSample = 0;
    config.endSample = 44100; // 1 second of audio
    config.tailDurationMs = 0;
    config.normalize = false;
    config.stemExport = false;

    uint64_t jobId = exportService->exportRangeAsync(config);
    REQUIRE(jobId > 0);

    // Wait for export to finish
    int attempts = 100;
    ExportProgress progress{};
    while (attempts-- > 0) {
        exportService->update();
        if (exportService->getProgress(jobId, progress)) {
            if (progress.status == ExportStatus::COMPLETED || progress.status == ExportStatus::FAILED) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    REQUIRE(progress.status == ExportStatus::COMPLETED);
    REQUIRE(std::filesystem::exists(outPath));

    // Verify exported audio with SndFileReader
    {
        SndFileReader reader(outPath);
        REQUIRE(reader.isValid());
        REQUIRE(reader.getNumChannels() == 2);
        REQUIRE(reader.getSampleRate() == 44100);
        REQUIRE(reader.getTotalFrames() >= 44100);

        std::vector<float> audioData(44100 * 2);
        uint32_t framesRead = reader.readFrames(audioData.data(), 44100);
        REQUIRE(framesRead == 44100);

        // Verify non-silent and check max peak
        float maxPeak = 0.0f;
        for (float s : audioData) {
            maxPeak = std::max(maxPeak, std::abs(s));
        }
        CHECK(maxPeak > 0.5f);
        CHECK(maxPeak <= 1.0f);

        // Check waveform samples match expected 50Hz sine wave
        for (uint32_t i = 0; i < 1000; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            float expectedVal = 0.8f * std::sin(2.0f * 3.14159265f * 50.0f * t);
            CHECK(audioData[i * 2 + 0] == Catch::Approx(expectedVal).margin(0.01f));
            CHECK(audioData[i * 2 + 1] == Catch::Approx(expectedVal).margin(0.01f));
        }
    }

    std::filesystem::remove(outPath);
}

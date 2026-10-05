#include <catch2/catch_test_macros.hpp>
#include "Agentic layer/server/handlers/rendering_handler.h"
#include "Agentic layer/common/parsed_args.h"
#include "Middle Bridge/engine/irender_controller.h"
#include "Middle Bridge/tracks/itrack_controller.h"
#include "Middle Bridge/timeline/itimeline_controller.h"
#include "Middle Bridge/timeline/iarrangement_controller.h"
#include <cstring>
#include <vector>

namespace {

class FakeRenderController : public bridge::IRenderController {
public:
    uint64_t enqueueRenderJob(const RenderConfiguration& config) override {
        lastConfig = config;
        RenderJobInfo info{};
        info.jobId = ++nextId;
        info.state = RenderJobState::PROCESSING;
        info.progress = 0.5f;
        info.isStemExport = config.stemExport;
        info.currentItemIndex = 1;
        info.totalItemCount = config.stemExport ? config.numStemTracks : 1;
        std::strncpy(info.jobName, config.stemExport ? "Stem Export" : "Full Mixdown", sizeof(info.jobName) - 1);
        std::strncpy(info.outputPath, config.outputFilePath, sizeof(info.outputPath) - 1);
        std::strncpy(info.statusMessage, "Processing...", sizeof(info.statusMessage) - 1);
        jobs.push_back(info);
        return info.jobId;
    }

    bool getJobInfo(uint64_t jobId, RenderJobInfo& outInfo) const override {
        for (const auto& j : jobs) {
            if (j.jobId == jobId) {
                outInfo = j;
                return true;
            }
        }
        return false;
    }

    std::vector<RenderJobInfo> listAllJobs() const override {
        return jobs;
    }

    void cancelJob(uint64_t jobId) override {
        for (auto& j : jobs) {
            if (j.jobId == jobId) {
                j.state = RenderJobState::CANCELLED;
            }
        }
    }

    void getSupportedCapabilities(
        std::vector<RenderFormat>& outFormats,
        std::vector<uint32_t>& outSampleRates,
        std::vector<uint8_t>& outBitDepths) const override {
        outFormats = { RenderFormat::WAV, RenderFormat::FLAC, RenderFormat::MP3 };
        outSampleRates = { 44100, 48000 };
        outBitDepths = { 16, 24, 32 };
    }

    void startOfflineRender(const RenderConfiguration& config) override {
        enqueueRenderJob(config);
    }
    bool isRenderingActive() const override { return false; }
    float getRenderProgress() const override { return 1.0f; }
    const char* getRenderStatusMessage() const override { return "Done"; }
    void cancelOfflineRender() override {}
    bool hasFailed(char*, uint32_t) const override { return false; }

    void startSilentMixAnalysis(uint64_t, uint64_t, uint32_t, uint32_t) override {}
    bool renderTrackToBufferSync(uint32_t, uint64_t, uint64_t, uint32_t, std::vector<float>&) override { return true; }

    mutable uint64_t nextId{0};
    mutable std::vector<RenderJobInfo> jobs;
    RenderConfiguration lastConfig{};
};

} // namespace

TEST_CASE("agentic::RenderingHandler Real Bridge Command Dispatch", "[agentic][export][job]") {
    FakeRenderController renderCtrl;

    SECTION("Fails cleanly when render controller is null") {
        auto args = agentic::ParsedArgs::parseCommandLine("export master --output /tmp/test.wav");
        auto res = agentic::RenderingHandler::handleCommand(args, nullptr);
        REQUIRE_FALSE(res.isSuccess());
        REQUIRE(res.code == agentic::ErrorCode::DAW_NOT_RUNNING);
    }

    SECTION("export master starts render job with expected configuration") {
        auto args = agentic::ParsedArgs::parseCommandLine("export master --output /tmp/mymix.wav --format wav --bit-depth 24 --sample-rate 48000 --normalize on --norm-db -0.5");
        auto res = agentic::RenderingHandler::handleCommand(args, &renderCtrl);
        REQUIRE(res.isSuccess());
        REQUIRE(res.symbol == "EXPORT_JOB_STARTED");
        REQUIRE(res.fields.at("type") == "MASTER_BOUNCE");
        REQUIRE(res.fields.at("destination") == "/tmp/mymix.wav");
        REQUIRE(res.fields.at("format") == "WAV");

        REQUIRE_FALSE(renderCtrl.lastConfig.stemExport);
        REQUIRE(renderCtrl.lastConfig.bitDepth == 24);
        REQUIRE(renderCtrl.lastConfig.sampleRate == 48000);
        REQUIRE(renderCtrl.lastConfig.normalize);
        REQUIRE(renderCtrl.lastConfig.normalizationdB == -0.5f);
    }

    SECTION("export stems enqueues stem export job") {
        auto args = agentic::ParsedArgs::parseCommandLine("export stems --output /tmp/stems/ --track 1..4 --format flac --bit-depth 16");
        auto res = agentic::RenderingHandler::handleCommand(args, &renderCtrl);
        REQUIRE(res.isSuccess());
        REQUIRE(res.symbol == "STEM_EXPORT_STARTED");
        REQUIRE(res.fields.at("type") == "STEM_EXPORT");
        REQUIRE(res.fields.at("total_tracks") == "4");

        REQUIRE(renderCtrl.lastConfig.stemExport);
        REQUIRE(renderCtrl.lastConfig.numStemTracks == 4);
        REQUIRE(renderCtrl.lastConfig.format == RenderFormat::FLAC);
        REQUIRE(renderCtrl.lastConfig.bitDepth == 16);
    }

    SECTION("job list returns queued/active jobs") {
        auto args = agentic::ParsedArgs::parseCommandLine("job list");
        auto res = agentic::RenderingHandler::handleCommand(args, &renderCtrl);
        REQUIRE(res.isSuccess());
        REQUIRE(res.symbol == "JOB_LIST");
        REQUIRE(res.rows.size() == 2);
    }

    SECTION("job status queries specific job by id") {
        auto args = agentic::ParsedArgs::parseCommandLine("job status --id 1");
        auto res = agentic::RenderingHandler::handleCommand(args, &renderCtrl);
        REQUIRE(res.isSuccess());
        REQUIRE(res.symbol == "JOB_STATUS");
        REQUIRE(res.fields.at("job_id") == "1");
        REQUIRE(res.fields.at("status") == "PROCESSING");
        REQUIRE(res.fields.at("progress") == "50%");
    }

    SECTION("job cancel marks job cancelled") {
        auto args = agentic::ParsedArgs::parseCommandLine("job cancel --id 1");
        auto res = agentic::RenderingHandler::handleCommand(args, &renderCtrl);
        REQUIRE(res.isSuccess());
        REQUIRE(res.symbol == "JOB_CANCELLED");

        RenderJobInfo info{};
        REQUIRE(renderCtrl.getJobInfo(1, info));
        REQUIRE(info.state == RenderJobState::CANCELLED);
    }
}

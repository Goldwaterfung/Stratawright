#include "rendering_handler.h"
#include "Middle Bridge/engine/irender_controller.h"
#include "Middle Bridge/tracks/itrack_controller.h"
#include "Middle Bridge/timeline/itimeline_controller.h"
#include "Middle Bridge/timeline/iarrangement_controller.h"
#include "common/system_primitives.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace agentic {

namespace {

std::string stateToString(RenderJobState state) {
    switch (state) {
        case RenderJobState::QUEUED:     return "QUEUED";
        case RenderJobState::PREPARING:  return "PREPARING";
        case RenderJobState::PROCESSING: return "PROCESSING";
        case RenderJobState::FINALIZING: return "FINALIZING";
        case RenderJobState::COMPLETED:  return "COMPLETED";
        case RenderJobState::FAILED:     return "FAILED";
        case RenderJobState::CANCELLED:  return "CANCELLED";
    }
    return "UNKNOWN";
}

RenderFormat parseFormat(std::string_view fmtStr) {
    if (fmtStr == "flac") return RenderFormat::FLAC;
    if (fmtStr == "mp3")  return RenderFormat::MP3;
    if (fmtStr == "aiff") return RenderFormat::AIFF;
    if (fmtStr == "ogg")  return RenderFormat::OGG;
    return RenderFormat::WAV;
}

std::string formatToString(RenderFormat fmt) {
    switch (fmt) {
        case RenderFormat::WAV:  return "WAV";
        case RenderFormat::FLAC: return "FLAC";
        case RenderFormat::MP3:  return "MP3";
        case RenderFormat::AIFF: return "AIFF";
        case RenderFormat::OGG:  return "OGG";
    }
    return "WAV";
}

} // namespace

ExecutionResult RenderingHandler::handleCommand(
    const ParsedArgs& args,
    bridge::IRenderController* renderController,
    bridge::ITrackController* trackController,
    bridge::ITimelineController* timelineController,
    bridge::IArrangementController* arrangementController)
{
    if (!renderController) {
        return ExecutionResult::Error(ErrorCode::DAW_NOT_RUNNING, "DAW_NOT_RUNNING", "RenderController is unavailable.");
    }

    std::string_view verb = args.getVerb();
    std::string_view sub = args.getSubcommand();

    // =========================================================================
    // 1. EXPORT MASTER / MIXDOWN
    // =========================================================================
    if (verb == "export" && (sub == "master" || sub == "mixdown" || sub.empty())) {
        std::string_view out = args.getOption("--output");
        if (out.empty()) {
            out = args.getOption("-o", "/tmp/mixdown.wav");
        }

        RenderConfiguration config{};
        std::strncpy(config.outputFilePath, out.data(), sizeof(config.outputFilePath) - 1);

        std::string_view fmtStr = args.getOption("--format", "wav");
        config.format = parseFormat(fmtStr);

        uint32_t bd = ParsedArgs::parseUint32(args.getOption("--bit-depth", "24")).value_or(24);
        config.bitDepth = static_cast<uint8_t>(bd == 16 || bd == 24 || bd == 32 ? bd : 24);

        uint32_t defaultSr = timelineController ? static_cast<uint32_t>(timelineController->getSampleRate()) : 48000;
        config.sampleRate = ParsedArgs::parseUint32(args.getOption("--sample-rate", "")).value_or(defaultSr);

        std::string_view ditherOpt = args.getOption("--dither", "on");
        config.enableDither = (ditherOpt == "on" || ditherOpt == "true" || args.hasFlag("--dither"));

        std::string_view splitOpt = args.getOption("--split-planar", "off");
        config.splitPlanar = (splitOpt == "on" || splitOpt == "true" || args.hasFlag("--split-planar"));

        std::string_view normOpt = args.getOption("--normalize", "off");
        config.normalize = (normOpt == "on" || normOpt == "true" || args.hasFlag("--normalize"));
        config.normalizationdB = ParsedArgs::parseFloat(args.getOption("--norm-db", "-0.1")).value_or(-0.1f);

        std::string_view rangeOpt = args.getOption("--range", "");
        bool useLoop = false;
        if (!rangeOpt.empty()) {
            useLoop = (rangeOpt == "loop");
        } else if (timelineController && timelineController->isLooping()) {
            useLoop = true;
        }

        if (useLoop && timelineController) {
            config.useLoopRegion = true;
            config.startFrame = timelineController->getLoopStart();
            config.endFrame = timelineController->getLoopEnd();
            if (config.endFrame <= config.startFrame) {
                uint64_t arrLen = arrangementController ? arrangementController->getArrangementLength() : 0;
                config.endFrame = (arrLen > 0) ? arrLen : config.sampleRate;
            }
        } else {
            config.useLoopRegion = false;
            config.startFrame = 0;
            config.tailDurationMs = ParsedArgs::parseUint32(args.getOption("--tail-ms", "1000")).value_or(1000);

            uint64_t arrLen = arrangementController ? arrangementController->getArrangementLength() : 0;
            if (arrLen > 0) {
                config.endFrame = arrLen;
            } else {
                double bpm = timelineController ? timelineController->getBPM() : 120.0;
                if (bpm <= 0.0) bpm = 120.0;
                uint64_t barFrames = static_cast<uint64_t>(static_cast<double>(config.sampleRate) * (4.0 * 60.0 / bpm));
                config.endFrame = barFrames;
            }
        }

        config.stemExport = false;
        config.numStemTracks = 0;

        uint64_t jobId = renderController->enqueueRenderJob(config);
        if (jobId == 0) {
            return ExecutionResult::Error(ErrorCode::PLUGIN_FAULT, "PLUGIN_FAULT", "Failed to enqueue master mixdown render job.");
        }

        return ExecutionResult::Success("EXPORT_JOB_STARTED", {
            {"job_id", std::to_string(jobId)},
            {"type", "MASTER_BOUNCE"},
            {"format", formatToString(config.format)},
            {"sample_rate", std::to_string(config.sampleRate)},
            {"bit_depth", std::to_string(config.bitDepth)},
            {"destination", std::string(config.outputFilePath)}
        });
    }

    // =========================================================================
    // 2. EXPORT STEMS
    // =========================================================================
    if (verb == "export" && sub == "stems") {
        std::string_view out = args.getOption("--output");
        if (out.empty()) {
            out = args.getOption("-o", "/tmp/stems/");
        }

        RenderConfiguration config{};
        std::strncpy(config.outputFilePath, out.data(), sizeof(config.outputFilePath) - 1);

        config.stemExport = true;

        std::string_view fmtStr = args.getOption("--format", "wav");
        config.format = parseFormat(fmtStr);

        uint32_t bd = ParsedArgs::parseUint32(args.getOption("--bit-depth", "24")).value_or(24);
        config.bitDepth = static_cast<uint8_t>(bd == 16 || bd == 24 || bd == 32 ? bd : 24);

        uint32_t defaultSr = timelineController ? static_cast<uint32_t>(timelineController->getSampleRate()) : 48000;
        config.sampleRate = ParsedArgs::parseUint32(args.getOption("--sample-rate", "")).value_or(defaultSr);

        std::string_view ditherOpt = args.getOption("--dither", "on");
        config.enableDither = (ditherOpt == "on" || ditherOpt == "true" || args.hasFlag("--dither"));

        std::string_view normOpt = args.getOption("--normalize", "off");
        config.normalize = (normOpt == "on" || normOpt == "true" || args.hasFlag("--normalize"));
        config.normalizationdB = ParsedArgs::parseFloat(args.getOption("--norm-db", "-0.1")).value_or(-0.1f);

        // Pattern
        std::string_view pattern = args.getOption("--naming-pattern", "{track_index:02d}_{track_name}");
        std::strncpy(config.stemNamingPattern, pattern.data(), sizeof(config.stemNamingPattern) - 1);

        // Routing policies
        config.printAuxReturnsIntoStems = args.hasFlag("--print-sends") || (args.getOption("--print-aux") == "on");
        config.activeSidechainsDuringExport = !args.hasFlag("--bypass-sidechain");
        config.allowMutedGhostSidechainTriggers = !args.hasFlag("--disable-ghost-triggers");

        // Timeline Bounds
        std::string_view rangeOpt = args.getOption("--range", "");
        bool useLoop = (rangeOpt == "loop");
        if (useLoop && timelineController) {
            config.useLoopRegion = true;
            config.startFrame = timelineController->getLoopStart();
            config.endFrame = timelineController->getLoopEnd();
            if (config.endFrame <= config.startFrame) {
                uint64_t arrLen = arrangementController ? arrangementController->getArrangementLength() : 0;
                config.endFrame = (arrLen > 0) ? arrLen : config.sampleRate;
            }
        } else {
            config.useLoopRegion = false;
            config.startFrame = 0;
            config.tailDurationMs = ParsedArgs::parseUint32(args.getOption("--tail-ms", "1000")).value_or(1000);

            uint64_t arrLen = arrangementController ? arrangementController->getArrangementLength() : 0;
            if (arrLen > 0) {
                config.endFrame = arrLen;
            } else {
                double bpm = timelineController ? timelineController->getBPM() : 120.0;
                if (bpm <= 0.0) bpm = 120.0;
                uint64_t barFrames = static_cast<uint64_t>(static_cast<double>(config.sampleRate) * (4.0 * 60.0 / bpm));
                config.endFrame = barFrames;
            }
        }

        // Track resolution
        std::string_view trackOpt = args.getOption("--track", "all");
        uint32_t count = 0;

        if (trackController) {
            auto tracks = trackController->getAllTracks();
            if (trackOpt == "all" || trackOpt.empty()) {
                for (size_t i = 0; i < tracks.size() && count < MAX_STEM_TRACKS; ++i) {
                    config.stemTrackIds[count++] = tracks[i].trackId.id;
                }
            } else {
                auto selectedIndices = ParsedArgs::parseIntegerRange(trackOpt);
                for (uint32_t idx : selectedIndices) {
                    if (idx >= 1 && idx <= tracks.size() && count < MAX_STEM_TRACKS) {
                        config.stemTrackIds[count++] = tracks[idx - 1].trackId.id;
                    }
                }
            }
        } else {
            if (trackOpt != "all" && !trackOpt.empty()) {
                auto selectedIndices = ParsedArgs::parseIntegerRange(trackOpt);
                for (uint32_t idx : selectedIndices) {
                    if (count < MAX_STEM_TRACKS) {
                        config.stemTrackIds[count++] = idx;
                    }
                }
            }
        }

        config.numStemTracks = count;
        if (config.numStemTracks == 0) {
            return ExecutionResult::Error(ErrorCode::ENTITY_NOT_FOUND, "ENTITY_NOT_FOUND", "No valid tracks selected for stem export.");
        }

        uint64_t jobId = renderController->enqueueRenderJob(config);
        if (jobId == 0) {
            return ExecutionResult::Error(ErrorCode::PLUGIN_FAULT, "PLUGIN_FAULT", "Failed to launch stem export job.");
        }

        return ExecutionResult::Success("STEM_EXPORT_STARTED", {
            {"job_id", std::to_string(jobId)},
            {"type", "STEM_EXPORT"},
            {"total_tracks", std::to_string(config.numStemTracks)},
            {"format", formatToString(config.format)},
            {"sample_rate", std::to_string(config.sampleRate)},
            {"destination", std::string(config.outputFilePath)}
        });
    }

    // =========================================================================
    // 3. JOB SUBCOMMANDS: list, status, cancel
    // =========================================================================
    if (verb == "job") {
        if (sub == "list") {
            auto jobs = renderController->listAllJobs();
            if (jobs.empty()) {
                return ExecutionResult::Success("JOB_LIST_EMPTY", {
                    {"message", "No render jobs found in queue."}
                });
            }

            std::vector<std::map<std::string, std::string>> rows;
            rows.reserve(jobs.size());

            for (const auto& job : jobs) {
                std::map<std::string, std::string> row{
                    {"JOB_ID", std::to_string(job.jobId)},
                    {"NAME", std::string(job.jobName)},
                    {"TYPE", job.isStemExport ? "STEM_EXPORT" : "MASTER_BOUNCE"},
                    {"STATUS", stateToString(job.state)},
                    {"PROGRESS", std::to_string(static_cast<int>(job.progress * 100.0f)) + "%"},
                    {"OUTPUT", std::string(job.outputPath)}
                };
                rows.push_back(row);
            }
            return ExecutionResult::MultiSuccess("JOB_LIST", rows);
        }

        if (sub == "status") {
            std::string_view idStr = args.getOption("--id");
            if (idStr.empty()) {
                return ExecutionResult::Error(ErrorCode::INVALID_ARGS, "INVALID_ARGS", "Missing required --id option.");
            }

            auto maybeId = ParsedArgs::parseUint32(idStr);
            if (!maybeId.has_value()) {
                return ExecutionResult::Error(ErrorCode::INVALID_ARGS, "INVALID_ARGS", "Invalid job id: " + std::string(idStr));
            }

            uint64_t jobId = static_cast<uint64_t>(maybeId.value());
            RenderJobInfo info{};
            if (!renderController->getJobInfo(jobId, info)) {
                return ExecutionResult::Error(ErrorCode::ENTITY_NOT_FOUND, "ENTITY_NOT_FOUND", "Render job #" + std::to_string(jobId) + " not found.");
            }

            std::map<std::string, std::string> fields{
                {"job_id", std::to_string(info.jobId)},
                {"name", std::string(info.jobName)},
                {"type", info.isStemExport ? "STEM_EXPORT" : "MASTER_BOUNCE"},
                {"status", stateToString(info.state)},
                {"progress", std::to_string(static_cast<int>(info.progress * 100.0f)) + "%"},
                {"status_message", std::string(info.statusMessage)},
                {"destination", std::string(info.outputPath)}
            };
            if (info.isStemExport && info.totalItemCount > 0) {
                fields["current_track_index"] = std::to_string(info.currentItemIndex);
                fields["total_tracks"] = std::to_string(info.totalItemCount);
            }

            return ExecutionResult::Success("JOB_STATUS", fields);
        }

        if (sub == "cancel") {
            std::string_view idStr = args.getOption("--id");
            if (idStr.empty()) {
                return ExecutionResult::Error(ErrorCode::INVALID_ARGS, "INVALID_ARGS", "Missing required --id option.");
            }

            auto maybeId = ParsedArgs::parseUint32(idStr);
            if (!maybeId.has_value()) {
                return ExecutionResult::Error(ErrorCode::INVALID_ARGS, "INVALID_ARGS", "Invalid job id: " + std::string(idStr));
            }

            uint64_t jobId = static_cast<uint64_t>(maybeId.value());
            renderController->cancelJob(jobId);

            return ExecutionResult::Success("JOB_CANCELLED", {
                {"job_id", std::to_string(jobId)}
            });
        }
    }

    return ExecutionResult::Error(ErrorCode::INVALID_ARGS, "INVALID_ARGS", "Unknown export/job command or missing arguments.");
}

} // namespace agentic


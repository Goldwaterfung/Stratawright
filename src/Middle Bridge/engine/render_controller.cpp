#include "Middle Bridge/engine/render_controller.h"
#include "Media management/export/iexport_service.h"
#include "Core infrastructure/memory/istring_registry.h"
#include "project/isession_manager.h"
#include "musical_composition/project_session/iproject_session.h"
#include "musical_composition/track_manager/itrack_manager.h"
#include "Core audio engine/engine/iaudio_engine.h"
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

namespace {

MediaManagement::ExportFormat toExportFormat(RenderFormat format) {
    switch (format) {
        case RenderFormat::WAV: return MediaManagement::ExportFormat::WAV;
        case RenderFormat::AIFF: return MediaManagement::ExportFormat::AIFF;
        case RenderFormat::FLAC: return MediaManagement::ExportFormat::FLAC;
        case RenderFormat::MP3: return MediaManagement::ExportFormat::MP3;
        case RenderFormat::OGG: return MediaManagement::ExportFormat::OGG;
    }
    return MediaManagement::ExportFormat::WAV;
}

MediaManagement::ExportBitDepth toExportBitDepth(uint8_t bitDepth) {
    switch (bitDepth) {
        case 16: return MediaManagement::ExportBitDepth::BIT_16;
        case 24: return MediaManagement::ExportBitDepth::BIT_24;
        case 32: return MediaManagement::ExportBitDepth::BIT_32_FLOAT;
        default: return MediaManagement::ExportBitDepth::BIT_24;
    }
}

std::string formatExtension(RenderFormat format) {
    switch (format) {
        case RenderFormat::WAV: return ".wav";
        case RenderFormat::AIFF: return ".aiff";
        case RenderFormat::FLAC: return ".flac";
        case RenderFormat::MP3: return ".mp3";
        case RenderFormat::OGG: return ".ogg";
    }
    return ".wav";
}

std::string sanitizeFilename(const std::string& name) {
    std::string safe = name;
    for (char& c : safe) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    return safe;
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t startPos = 0;
    while ((startPos = str.find(from, startPos)) != std::string::npos) {
        str.replace(startPos, from.length(), to);
        startPos += to.length();
    }
}

std::string formatStemFilename(
    const char* patternInput,
    uint32_t trackNum,
    uint32_t stemIndex,
    const std::string& trackName,
    const std::string& projectName,
    RenderFormat format)
{
    std::string pattern = (patternInput && patternInput[0] != '\0')
        ? std::string(patternInput)
        : "{track_index:02d}_{track_name}";

    char index02d[16];
    std::snprintf(index02d, sizeof(index02d), "%02u", trackNum);
    std::string indexStr = std::to_string(trackNum);

    char stem02d[16];
    std::snprintf(stem02d, sizeof(stem02d), "%02u", stemIndex);
    std::string stemIndexStr = std::to_string(stemIndex);

    std::string safeTrack = sanitizeFilename(trackName.empty() ? ("Track_" + indexStr) : trackName);
    std::string safeProj = sanitizeFilename(projectName.empty() ? "Project" : projectName);
    std::string ext = formatExtension(format);
    std::string extNoDot = ext.empty() ? "" : ext.substr(1);

    replaceAll(pattern, "{track_index:02d}", index02d);
    replaceAll(pattern, "{track_index}", indexStr);
    replaceAll(pattern, "{index:02d}", index02d);
    replaceAll(pattern, "{index}", indexStr);
    replaceAll(pattern, "{stem_index:02d}", stem02d);
    replaceAll(pattern, "{stem_index}", stemIndexStr);
    replaceAll(pattern, "{track_name}", safeTrack);
    replaceAll(pattern, "{project}", safeProj);
    replaceAll(pattern, "{ext}", extNoDot);

    if (pattern.length() < ext.length() || pattern.compare(pattern.length() - ext.length(), ext.length(), ext) != 0) {
        pattern += ext;
    }
    return pattern;
}

} // anonymous namespace

namespace bridge {

RenderController::RenderController(MediaManagement::IExportService* exportService, Layer2::IStringRegistry* stringRegistry, ISessionManager* sessionManager)
    : m_exportService(exportService)
    , m_stringRegistry(stringRegistry)
    , m_sessionManager(sessionManager)
{
}

RenderController::~RenderController() = default;

uint64_t RenderController::enqueueRenderJob(const RenderConfiguration& config)
{
    if (!m_exportService || !m_stringRegistry) return 0;

    MediaManagement::ExportConfig exportConfig{};
    exportConfig.outputPathId = m_stringRegistry->registerString(config.outputFilePath);
    exportConfig.startSample = config.startFrame;
    exportConfig.endSample = config.endFrame;
    exportConfig.sampleRate = config.sampleRate;
    exportConfig.numChannels = 2; // Default to stereo
    exportConfig.format = toExportFormat(config.format);
    exportConfig.bitDepth = toExportBitDepth(config.bitDepth);
    exportConfig.normalize = config.normalize;
    exportConfig.normalizationdB = config.normalizationdB;
    exportConfig.dither = config.enableDither ? MediaManagement::DitherType::TPDF : MediaManagement::DitherType::NONE;
    exportConfig.stemExport = config.stemExport;
    exportConfig.numStemNodes = config.numStemTracks;
    exportConfig.tailDurationMs = config.tailDurationMs;
    exportConfig.printAuxReturnsIntoStems = config.printAuxReturnsIntoStems;
    exportConfig.activeSidechainsDuringExport = config.activeSidechainsDuringExport;
    exportConfig.allowMutedGhostSidechainTriggers = config.allowMutedGhostSidechainTriggers;

    exportConfig.titleId = 0;
    exportConfig.artistId = 0;
    exportConfig.albumId = 0;
    exportConfig.genreId = 0;
    exportConfig.commentId = 0;

    std::string projectName = "Project";
    composition::ITrackManager* tm = nullptr;
    if (m_sessionManager) {
        if (auto* session = m_sessionManager->getActiveSession()) {
            projectName = session->getMetadata().projectName;
            tm = session->getTrackManager();
        }
    }

    if (config.stemExport && config.numStemTracks > 0) {
        uint32_t count = std::min(config.numStemTracks, MAX_STEM_TRACKS);
        exportConfig.numStemNodes = count;

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t rawTrackId = config.stemTrackIds[i];
            TrackID tid;
            tid.id = rawTrackId;
            tid.generation = 1;

            std::string trackName;
            uint32_t trackNum = i + 1;

            if (tm) {
                composition::TrackCreateInfo tInfo{};
                if (tm->getTrackInfo(tid, tInfo)) {
                    if (tInfo.nameId != 0) {
                        m_stringRegistry->getString(tInfo.nameId, trackName);
                    }
                }
                uint32_t pos = tm->getTrackIndexPosition(tid);
                if (pos > 0) {
                    trackNum = pos;
                } else if (rawTrackId > 0) {
                    trackNum = rawTrackId;
                }

                NodeID node = tm->getTrackOutputNode(tid);
                if (node.isValid()) {
                    exportConfig.stemNodes[i] = node;
                } else {
                    exportConfig.stemNodes[i] = NodeID{rawTrackId, 1};
                }
            } else {
                exportConfig.stemNodes[i] = NodeID{rawTrackId, 1};
            }

            std::string fname = formatStemFilename(
                config.stemNamingPattern,
                trackNum,
                i + 1,
                trackName,
                projectName,
                config.format
            );

            std::strncpy(exportConfig.stemFileNames[i], fname.c_str(), sizeof(exportConfig.stemFileNames[i]) - 1);
            exportConfig.stemFileNames[i][sizeof(exportConfig.stemFileNames[i]) - 1] = '\0';
        }
    }

    m_hasFailed = false;
    m_lastError[0] = '\0';
    m_progress = 0.0f;
    m_statusMsg = "Preparing...";

    if (m_audioEngine) {
        m_audioEngine->setOfflineExportActive(true);
    }

    uint64_t jobId = m_exportService->exportRangeAsync(exportConfig, &RenderController::onExportCompleted, this);
    if (jobId == 0) {
        if (m_audioEngine) {
            m_audioEngine->setOfflineExportActive(false);
        }
        return 0;
    }

    m_activeJobId = jobId;
    m_hasActiveJob = true;

    RenderJobInfo info{};
    info.jobId = jobId;
    info.state = RenderJobState::PREPARING;
    info.progress = 0.0f;
    info.isStemExport = config.stemExport;
    info.currentItemIndex = 1;
    info.totalItemCount = config.stemExport ? (config.numStemTracks > 0 ? config.numStemTracks : 1) : 1;

    if (config.stemExport) {
        std::snprintf(info.jobName, sizeof(info.jobName), "Stem Export (%u tracks)", exportConfig.numStemNodes);
    } else {
        std::snprintf(info.jobName, sizeof(info.jobName), "Full Mixdown");
    }
    std::strncpy(info.outputPath, config.outputFilePath, sizeof(info.outputPath) - 1);
    info.outputPath[sizeof(info.outputPath) - 1] = '\0';
    std::strncpy(info.statusMessage, "Preparing...", sizeof(info.statusMessage) - 1);
    info.statusMessage[sizeof(info.statusMessage) - 1] = '\0';

    {
        std::lock_guard lock(m_jobsMutex);
        m_jobs[jobId] = info;
        m_jobOrder.push_back(jobId);
    }

    return jobId;
}

void RenderController::startOfflineRender(const RenderConfiguration& config)
{
    enqueueRenderJob(config);
}

void RenderController::updateJobInfoLocked(uint64_t jobId, RenderJobInfo& job) const
{
    if (!m_exportService) return;

    MediaManagement::ExportProgress progressStruct{};
    if (m_exportService->getProgress(jobId, progressStruct)) {
        job.progress = progressStruct.progress;
        job.currentItemIndex = progressStruct.currentItem;
        if (progressStruct.totalItems > 0) {
            job.totalItemCount = progressStruct.totalItems;
        }

        switch (progressStruct.status) {
            case MediaManagement::ExportStatus::PENDING:
                job.state = RenderJobState::QUEUED;
                std::snprintf(job.statusMessage, sizeof(job.statusMessage), "Queued");
                break;
            case MediaManagement::ExportStatus::PREPARING:
                job.state = RenderJobState::PREPARING;
                std::snprintf(job.statusMessage, sizeof(job.statusMessage), "Preparing render...");
                break;
            case MediaManagement::ExportStatus::PROCESSING:
                job.state = RenderJobState::PROCESSING;
                if (job.isStemExport && job.totalItemCount > 1) {
                    if (progressStruct.currentItemName[0] != '\0') {
                        std::snprintf(job.statusMessage, sizeof(job.statusMessage),
                                      "Rendering stem %u/%u (%s)...",
                                      progressStruct.currentItem, job.totalItemCount,
                                      progressStruct.currentItemName);
                    } else {
                        std::snprintf(job.statusMessage, sizeof(job.statusMessage),
                                      "Rendering stem %u/%u...",
                                      progressStruct.currentItem, job.totalItemCount);
                    }
                } else {
                    std::snprintf(job.statusMessage, sizeof(job.statusMessage), "Rendering audio blocks...");
                }
                break;
            case MediaManagement::ExportStatus::FINALIZING:
                job.state = RenderJobState::FINALIZING;
                std::snprintf(job.statusMessage, sizeof(job.statusMessage), "Finalizing output...");
                break;
            case MediaManagement::ExportStatus::COMPLETED:
                job.state = RenderJobState::COMPLETED;
                job.progress = 1.0f;
                std::snprintf(job.statusMessage, sizeof(job.statusMessage), "Render complete");
                break;
            case MediaManagement::ExportStatus::FAILED:
                job.state = RenderJobState::FAILED;
                if (progressStruct.errorMessage[0] != '\0') {
                    std::snprintf(job.statusMessage, sizeof(job.statusMessage), "Failed: %s", progressStruct.errorMessage);
                } else {
                    std::snprintf(job.statusMessage, sizeof(job.statusMessage), "Render failed");
                }
                break;
            case MediaManagement::ExportStatus::CANCELLED:
                job.state = RenderJobState::CANCELLED;
                std::snprintf(job.statusMessage, sizeof(job.statusMessage), "Render cancelled");
                break;
        }
    }
}

bool RenderController::getJobInfo(uint64_t jobId, RenderJobInfo& outInfo) const
{
    if (m_exportService) {
        m_exportService->update();
    }

    std::lock_guard lock(m_jobsMutex);
    auto it = m_jobs.find(jobId);
    if (it == m_jobs.end()) {
        return false;
    }

    updateJobInfoLocked(jobId, it->second);
    outInfo = it->second;
    return true;
}

std::vector<RenderJobInfo> RenderController::listAllJobs() const
{
    if (m_exportService) {
        m_exportService->update();
    }

    std::lock_guard lock(m_jobsMutex);
    std::vector<RenderJobInfo> result;
    result.reserve(m_jobOrder.size());

    for (uint64_t jobId : m_jobOrder) {
        auto it = m_jobs.find(jobId);
        if (it != m_jobs.end()) {
            updateJobInfoLocked(jobId, it->second);
            result.push_back(it->second);
        }
    }
    return result;
}

void RenderController::cancelJob(uint64_t jobId)
{
    if (m_exportService) {
        m_exportService->cancelExport(jobId);
    }

    if (m_activeJobId == jobId) {
        m_hasActiveJob = false;
        if (m_audioEngine) {
            m_audioEngine->setOfflineExportActive(false);
        }
    }

    std::lock_guard lock(m_jobsMutex);
    auto it = m_jobs.find(jobId);
    if (it != m_jobs.end()) {
        it->second.state = RenderJobState::CANCELLED;
        std::snprintf(it->second.statusMessage, sizeof(it->second.statusMessage), "Render cancelled");
    }
}

void RenderController::getSupportedCapabilities(
    std::vector<RenderFormat>& outFormats,
    std::vector<uint32_t>& outSampleRates,
    std::vector<uint8_t>& outBitDepths) const
{
    outFormats.clear();
    outSampleRates.clear();
    outBitDepths.clear();

    if (!m_exportService) return;

    const RenderFormat candidateFormats[] = {
        RenderFormat::WAV,
        RenderFormat::AIFF,
        RenderFormat::FLAC,
        RenderFormat::MP3,
        RenderFormat::OGG
    };
    for (auto fmt : candidateFormats) {
        if (m_exportService->isFormatSupported(toExportFormat(fmt))) {
            outFormats.push_back(fmt);
        }
    }

    uint32_t rates[16];
    uint32_t rateCount = sizeof(rates) / sizeof(rates[0]);
    m_exportService->getSupportedSampleRates(rates, &rateCount);
    for (uint32_t i = 0; i < rateCount; ++i) {
        outSampleRates.push_back(rates[i]);
    }

    MediaManagement::ExportBitDepth depths[8];
    uint32_t depthCount = sizeof(depths) / sizeof(depths[0]);
    m_exportService->getSupportedBitDepths(MediaManagement::ExportFormat::WAV, depths, &depthCount);
    for (uint32_t i = 0; i < depthCount; ++i) {
        switch (depths[i]) {
            case MediaManagement::ExportBitDepth::BIT_16:
                outBitDepths.push_back(16);
                break;
            case MediaManagement::ExportBitDepth::BIT_24:
                outBitDepths.push_back(24);
                break;
            case MediaManagement::ExportBitDepth::BIT_32_FLOAT:
                outBitDepths.push_back(32);
                break;
        }
    }
}

bool RenderController::isRenderingActive() const
{
    if (m_exportService) {
        m_exportService->update();
    }

    if (m_hasActiveJob) {
        MediaManagement::ExportProgress progressStruct{};
        if (m_exportService && m_exportService->getProgress(m_activeJobId, progressStruct)) {
            if (progressStruct.status == MediaManagement::ExportStatus::COMPLETED ||
                progressStruct.status == MediaManagement::ExportStatus::FAILED ||
                progressStruct.status == MediaManagement::ExportStatus::CANCELLED) {
                m_hasActiveJob = false;
                if (m_audioEngine) {
                    m_audioEngine->setOfflineExportActive(false);
                }
            }
        }
    }
    return m_hasActiveJob;
}

float RenderController::getRenderProgress() const
{
    if (m_exportService) {
        m_exportService->update();
    }

    if (!m_hasActiveJob) {
        return m_hasFailed ? 0.0f : 1.0f;
    }

    MediaManagement::ExportProgress progressStruct{};
    if (m_exportService && m_exportService->getProgress(m_activeJobId, progressStruct)) {
        m_progress = progressStruct.progress;
    }
    return m_progress;
}

const char* RenderController::getRenderStatusMessage() const
{
    if (m_exportService) {
        m_exportService->update();
    }

    if (!m_hasActiveJob) {
        if (m_hasFailed) {
            return "Render failed";
        }
        return "Idle";
    }

    RenderJobInfo info{};
    if (getJobInfo(m_activeJobId, info)) {
        m_statusMsg = info.statusMessage;
    }
    return m_statusMsg.c_str();
}

void RenderController::cancelOfflineRender()
{
    if (m_hasActiveJob) {
        cancelJob(m_activeJobId);
    } else if (m_exportService && m_activeJobId != 0) {
        cancelJob(m_activeJobId);
    }
}

bool RenderController::hasFailed(char* outError, uint32_t maxLen) const
{
    if (m_exportService) {
        m_exportService->update();
    }

    if (m_hasFailed) {
        std::strncpy(outError, m_lastError, maxLen - 1);
        outError[maxLen - 1] = '\0';
        return true;
    }
    return false;
}

void RenderController::onExportCompleted(uint64_t jobId, bool success, const char* error, void* context)
{
    auto* self = static_cast<RenderController*>(context);
    if (!self) return;

    if (self->m_activeJobId == jobId) {
        self->m_hasActiveJob = false;
        if (self->m_audioEngine) {
            self->m_audioEngine->setOfflineExportActive(false);
        }
        if (!success) {
            self->m_hasFailed = true;
            if (error) {
                std::strncpy(self->m_lastError, error, sizeof(self->m_lastError) - 1);
                self->m_lastError[sizeof(self->m_lastError) - 1] = '\0';
            } else {
                std::strcpy(self->m_lastError, "Export failed");
            }
        }
    }

    std::lock_guard lock(self->m_jobsMutex);
    auto it = self->m_jobs.find(jobId);
    if (it != self->m_jobs.end()) {
        if (success) {
            it->second.state = RenderJobState::COMPLETED;
            it->second.progress = 1.0f;
            std::strncpy(it->second.statusMessage, "Render complete", sizeof(it->second.statusMessage) - 1);
            it->second.statusMessage[sizeof(it->second.statusMessage) - 1] = '\0';
        } else {
            it->second.state = RenderJobState::FAILED;
            if (error) {
                std::snprintf(it->second.statusMessage, sizeof(it->second.statusMessage), "Failed: %s", error);
            } else {
                std::strncpy(it->second.statusMessage, "Render failed", sizeof(it->second.statusMessage) - 1);
                it->second.statusMessage[sizeof(it->second.statusMessage) - 1] = '\0';
            }
        }
    }
}

void RenderController::startSilentMixAnalysis(uint64_t startFrame, uint64_t endFrame, uint32_t sampleRate, uint32_t isolateTrackId) {
    if (!m_exportService) return;

    m_hasFailed = false;
    m_lastError[0] = '\0';
    m_progress = 0.0f;
    m_statusMsg = "Preparing analysis...";

    if (m_audioEngine) {
        m_audioEngine->setOfflineExportActive(true);
    }

    m_activeJobId = m_exportService->analyzeSessionLoudnessAsync(
        startFrame,
        endFrame,
        sampleRate,
        2, // Stereo
        isolateTrackId,
        &RenderController::onAnalysisCompleted,
        this
    );
    m_hasActiveJob = true;

    RenderJobInfo info{};
    info.jobId = m_activeJobId;
    info.state = RenderJobState::PROCESSING;
    info.progress = 0.0f;
    info.isStemExport = false;
    info.currentItemIndex = 1;
    info.totalItemCount = 1;
    std::snprintf(info.jobName, sizeof(info.jobName), "Mix Analysis");
    std::snprintf(info.outputPath, sizeof(info.outputPath), "Memory");
    std::snprintf(info.statusMessage, sizeof(info.statusMessage), "Analyzing loudness...");

    {
        std::lock_guard lock(m_jobsMutex);
        m_jobs[m_activeJobId] = info;
        m_jobOrder.push_back(m_activeJobId);
    }
}

void RenderController::onAnalysisCompleted(
    uint64_t jobId,
    bool success,
    const MediaManagement::IExportService::AnalysisResult& result,
    const char* error,
    void* context
) {
    auto* self = static_cast<RenderController*>(context);
    if (!self) return;

    if (self->m_activeJobId == jobId) {
        self->m_hasActiveJob = false;
        if (self->m_audioEngine) {
            self->m_audioEngine->setOfflineExportActive(false);
        }
        if (success) {
            if (self->m_sessionManager) {
                if (auto* session = self->m_sessionManager->getActiveSession()) {
                    composition::MixStatistics stats{};
                    stats.isAnalyzed = true;
                    stats.integratedLoudnessLUFS = result.integratedLoudnessLUFS;
                    stats.truePeakDBTP = result.truePeakDBTP;
                    stats.clippingDetected = result.clippingDetected;
                    stats.samplePeakDBFS = result.samplePeakDBFS;
                    stats.midRmsDbfs = result.midRmsDbfs;
                    stats.sideRmsDbfs = result.sideRmsDbfs;
                    stats.msRatioDb = result.msRatioDb;
                    stats.stereoWidthPct = result.stereoWidthPct;
                    stats.monoFoldLossDb = result.monoFoldLossDb;
                    stats.stereoCorrelation = result.stereoCorrelation;
                    session->setMixStatistics(stats);
                }
            }
        } else {
            self->m_hasFailed = true;
            if (error) {
                std::strncpy(self->m_lastError, error, sizeof(self->m_lastError) - 1);
                self->m_lastError[sizeof(self->m_lastError) - 1] = '\0';
            } else {
                std::strcpy(self->m_lastError, "Analysis failed");
            }
        }
    }

    std::lock_guard lock(self->m_jobsMutex);
    auto it = self->m_jobs.find(jobId);
    if (it != self->m_jobs.end()) {
        if (success) {
            it->second.state = RenderJobState::COMPLETED;
            it->second.progress = 1.0f;
            std::strncpy(it->second.statusMessage, "Analysis complete", sizeof(it->second.statusMessage) - 1);
            it->second.statusMessage[sizeof(it->second.statusMessage) - 1] = '\0';
        } else {
            it->second.state = RenderJobState::FAILED;
            if (error) {
                std::snprintf(it->second.statusMessage, sizeof(it->second.statusMessage), "Failed: %s", error);
            } else {
                std::strncpy(it->second.statusMessage, "Analysis failed", sizeof(it->second.statusMessage) - 1);
                it->second.statusMessage[sizeof(it->second.statusMessage) - 1] = '\0';
            }
        }
    }
}

bool RenderController::renderTrackToBufferSync(uint32_t trackId, uint64_t startFrame, uint64_t endFrame, uint32_t sampleRate, std::vector<float>& outBuffer) {
    if (!m_exportService) return false;
    return m_exportService->renderTrackToBufferSync(trackId, startFrame, endFrame, sampleRate, outBuffer);
}

void RenderController::setAudioEngine(Layer3::IAudioEngine* engine) {
    m_audioEngine = engine;
}

} // namespace bridge

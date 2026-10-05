#pragma once
#include "Middle Bridge/engine/irender_controller.h"
#include "Media management/export/iexport_service.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace Layer2 { class IStringRegistry; }
namespace Layer3 { class IAudioEngine; }
namespace MediaManagement { class IExportService; }

namespace bridge {
class ISessionManager;

class RenderController : public IRenderController {
public:
    RenderController(MediaManagement::IExportService* exportService, Layer2::IStringRegistry* stringRegistry, ISessionManager* sessionManager);
    ~RenderController() override;

    // IRenderController overrides
    uint64_t enqueueRenderJob(const RenderConfiguration& config) override;
    bool getJobInfo(uint64_t jobId, RenderJobInfo& outInfo) const override;
    std::vector<RenderJobInfo> listAllJobs() const override;
    void cancelJob(uint64_t jobId) override;

    void getSupportedCapabilities(
        std::vector<RenderFormat>& outFormats,
        std::vector<uint32_t>& outSampleRates,
        std::vector<uint8_t>& outBitDepths) const override;

    void startOfflineRender(const RenderConfiguration& config) override;
    bool isRenderingActive() const override;
    float getRenderProgress() const override;
    const char* getRenderStatusMessage() const override;
    void cancelOfflineRender() override;
    bool hasFailed(char* outError, uint32_t maxLen) const override;
    void startSilentMixAnalysis(uint64_t startFrame, uint64_t endFrame, uint32_t sampleRate, uint32_t isolateTrackId = 0) override;
    bool renderTrackToBufferSync(uint32_t trackId, uint64_t startFrame, uint64_t endFrame, uint32_t sampleRate, std::vector<float>& outBuffer) override;

    void setAudioEngine(Layer3::IAudioEngine* engine);

private:
    MediaManagement::IExportService* m_exportService = nullptr;
    Layer2::IStringRegistry* m_stringRegistry = nullptr;
    ISessionManager* m_sessionManager = nullptr;
    Layer3::IAudioEngine* m_audioEngine = nullptr;

    mutable std::mutex m_jobsMutex;
    mutable std::unordered_map<uint64_t, RenderJobInfo> m_jobs;
    std::vector<uint64_t> m_jobOrder;

    mutable uint64_t m_activeJobId = 0;
    mutable bool m_hasActiveJob = false;
    mutable float m_progress = 0.0f;
    mutable std::string m_statusMsg;
    mutable bool m_hasFailed = false;
    mutable char m_lastError[256] = "";

    void updateJobInfoLocked(uint64_t jobId, RenderJobInfo& job) const;

    static void onExportCompleted(uint64_t jobId, bool success, const char* error, void* context);
    static void onAnalysisCompleted(uint64_t jobId, bool success, const MediaManagement::IExportService::AnalysisResult& result, const char* error, void* context);
};

} // namespace bridge

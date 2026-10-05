#pragma once
#include "common/system_primitives.h"

#include <vector>
#include <cstdint>

namespace bridge {

/**
 * @brief Controller interface managing the offline rendering and export pipeline.
 * Coordinates with the background bounce engine and tracks rendering progress/errors.
 */
class IRenderController {
public:
    virtual ~IRenderController() = default;

    // Asynchronous Queue API
    virtual uint64_t enqueueRenderJob(const RenderConfiguration& config) = 0;
    virtual bool getJobInfo(uint64_t jobId, RenderJobInfo& outInfo) const = 0;
    virtual std::vector<RenderJobInfo> listAllJobs() const = 0;
    virtual void cancelJob(uint64_t jobId) = 0;

    // Capability Queries (replaces hardcoded UI lists)
    virtual void getSupportedCapabilities(
        std::vector<RenderFormat>& outFormats,
        std::vector<uint32_t>& outSampleRates,
        std::vector<uint8_t>& outBitDepths) const = 0;

    // Single-Job Compatibility
    virtual void startOfflineRender(const RenderConfiguration& config) = 0;
    virtual bool isRenderingActive() const = 0;
    virtual float getRenderProgress() const = 0;           // Range [0.0f, 1.0f]
    virtual const char* getRenderStatusMessage() const = 0; // Current step detail
    virtual void cancelOfflineRender() = 0;
    virtual bool hasFailed(char* outError, uint32_t maxLen) const = 0;

    // Analysis & Direct Buffers
    virtual void startSilentMixAnalysis(uint64_t startFrame, uint64_t endFrame, uint32_t sampleRate, uint32_t isolateTrackId = 0) = 0;
    virtual bool renderTrackToBufferSync(uint32_t trackId, uint64_t startFrame, uint64_t endFrame, uint32_t sampleRate, std::vector<float>& outBuffer) = 0;
};

} // namespace bridge

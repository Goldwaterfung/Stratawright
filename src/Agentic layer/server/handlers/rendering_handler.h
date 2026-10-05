#pragma once

#include "../../common/ipc_protocol.h"
#include "../../common/parsed_args.h"

namespace bridge {
class IRenderController;
class ITrackController;
class ITimelineController;
class IArrangementController;
}

namespace agentic {

class RenderingHandler {
public:
    static ExecutionResult handleCommand(
        const ParsedArgs& args,
        bridge::IRenderController* renderController,
        bridge::ITrackController* trackController = nullptr,
        bridge::ITimelineController* timelineController = nullptr,
        bridge::IArrangementController* arrangementController = nullptr
    );
};

} // namespace agentic

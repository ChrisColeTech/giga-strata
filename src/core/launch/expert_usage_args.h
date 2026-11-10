#pragma once

#include "common/result.h"
#include "launch/run_environment.h"
#include "launch/run_settings.h"
#include "plan/plan_request.h"

#include <QStringList>

#include <optional>

// What per_expert_args decided: its "[MOE] ..." log lines and, when per-expert placement is on
// and usable, the planner's per-expert request (counts, cache path, cached file).
struct ExpertUsagePlan {
    QStringList logLines;
    std::optional<PerExpertRequest> request;
};

// llama_launcher.per_expert_args with usage_summary and the naming half of hot_cache_path. The
// pruning half is Run-time I/O (02 D1); the hot path is in the request for it.
class ExpertUsageArgs {
public:
    // reserveBytes: the draft + projector reserve, part of the hot-cache key.
    static Result<ExpertUsagePlan> decide(const RunSettings &settings, const RunEnvironment &environment,
                                          qint64 reserveBytes);
};

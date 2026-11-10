#pragma once

#include "common/result.h"
#include "hardware/hardware_snapshot.h"
#include "plan/plan_model.h"
#include "plan/plan_request.h"

#include <atomic>

// strata_planner.make_plan: layer split and expert fill (via LayerPlacer), the auto micro-batch
// choice, per-expert placement when requested (via ExpertPlacer), the RAM/SSD decision and the
// llama-server arguments. Pure (02 D1): the hot file comes back as text for Run to write.
class PlacementPlanner {
public:
    // `cancel` stops a superseded live re-plan between knapsack layers (02 D5).
    static Result<Plan> plan(const PlanRequest &request, const HardwareSnapshot &hardware,
                             const std::atomic<bool> *cancel = nullptr);
};

#pragma once

#include "common/result.h"
#include "gguf/model_layout.h"
#include "moe/hot_map.h"
#include "plan/layer_placement.h"
#include "plan/plan_request.h"

#include <QByteArray>
#include <QStringList>
#include <QVector>

#include <atomic>

// What the per-expert branch of make_plan changes in a whole-layer placement.
struct ExpertPlacement {
    HotMap hot;
    bool isReused = false;
    QStringList reportLines;               // "Cost-aware placement: ..." and "Per-expert placement: ..."
    QVector<QVector<int>> gpuExpertLayers; // whole layers still placed by -ot (layers not in hot)
    QVector<int> cpuExpertLayers;          // whole layers still in RAM (layers not in hot)
    qint64 hotExpertBytes = 0;             // [hot_bytes] the VRAM share of the hot file's layers
    qint64 gpuExpertBytes = 0;            // [gpu_exp_bytes] hot experts plus whole GPU layers
    qint64 coldBytes = 0;                  // the RAM-tier share of the hot file's layers
};

// strata_planner.make_plan, per-expert branch (P5b): turns a whole-layer placement into a
// --moe-hot placement from recorded usage. Reuses the cached hot file when it still fits;
// otherwise builds one, in cost mode with the first-GPU reserve loop and the whole-layer check.
class ExpertPlacer {
public:
    static Result<ExpertPlacement> place(const ModelLayout &layout, const LayerPlacement &placement,
                                         const PerExpertRequest &request, const std::atomic<bool> *cancel = nullptr);

    // max(experts[il] * (n_expert - len(es)) // n_expert) over the split layers of `hot`, or 0:
    // prompt batches copy a split layer's whole cold side to the first GPU.
    static qint64 largestColdSide(const ModelLayout &layout, const HotMap &hot);

    // experts[il] * len(es) // n_expert: the VRAM share of one layer's hot experts (02 H3).
    static qint64 hotBytes(const ModelLayout &layout, int layer, qsizetype hotCount);
};

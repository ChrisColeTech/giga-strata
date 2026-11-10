#pragma once

#include "common/result.h"
#include "gguf/model_layout.h"
#include "moe/usage_counts.h"

#include <QByteArray>
#include <QString>
#include <QVector>

#include <optional>

// make_plan's moe_hot_* keywords as llama_launcher.per_expert_args passes them. The caller reads
// the files (02 D1: planning reads, it never writes); the planner only decides.
struct PerExpertRequest {
    // read_counts(moe_hot_counts). An error is kept rather than reported at once because make_plan
    // reads the counts only after the GPU and VRAM checks, and those errors come first.
    Result<UsageCounts> counts = Result<UsageCounts>::success(UsageCounts());
    QString hotFilePath;                      // [moe_hot_path] the cache file Run writes
    bool isCostAware = true;                  // [moe_hot_cost]
    std::optional<QByteArray> cachedHotFile;  // [moe_hot_reuse] hotFilePath's text when it exists
};

// Arguments of strata_planner.make_plan, with the model already read (02 D4: layouts are cached
// per model) and hardware passed separately as a HardwareSnapshot.
struct PlanRequest {
    ModelLayout layout;
    QVector<int> gpuIds;              // nvidia-smi indices in CUDA0, CUDA1, ... order
    qint64 contextSize = 0;
    qint64 marginMib = 0;
    bool allowSsd = true;
    std::optional<int> microBatch;    // nullopt = "auto"
    qint64 reserveLastBytes = 0;      // draft + vision projector reserve on the last GPU
    std::optional<PerExpertRequest> perExpert;  // nullopt places whole layers only
};

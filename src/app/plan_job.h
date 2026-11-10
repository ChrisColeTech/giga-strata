#pragma once

#include "hardware/hardware_snapshot.h"
#include "launch/run_planner.h"
#include "launch/run_settings.h"

#include <QSet>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <atomic>
#include <memory>
#include <optional>

class ServerBuild;
class ShardCache;

// The data folders and shared engine pieces every plan reads.
struct PlanningContext {
    ServerBuild *build = nullptr;
    ShardCache *shards = nullptr;
    QString modelsDirectory;
    QString usageDirectory;
    QString hotDirectory;
};

// Everything one plan needs, copied so it can run on a worker thread (R27).
struct PlanJobInput {
    QVariantMap values;          // the model's parameters plus model, host, port, binary
    QString binary;
    HardwareSnapshot hardware;
    QString modelsDirectory;
    QString usageDirectory;
    QString hotDirectory;
    bool isRun = false;          // Run uses the build's real flags; a live preview may assume them (02 D3)
    ServerBuild *build = nullptr;
    ShardCache *shards = nullptr;
    std::shared_ptr<std::atomic<bool>> cancel;
};

struct PlanJobResult {
    std::optional<RunSettings> settings;
    QSet<QString> flags;
    RunPlan run;
    QString error;               // settings or plan failure in words; empty when planned
    QVariantMap modelInfo;       // type, layers, experts, sizeBytes, files, draftPath, projectorPath, decodeTokens, promptTokens
    QVariantList tiers;
    QString summary;
};

// One pass of the Run path (RunPlanner) with the live view's extras, on the real filesystem.
// Shared by PlannerRunner (live preview) and LaunchController (Run).
class PlanJob {
public:
    static PlanJobResult run(const PlanJobInput &input);

    // The input for `values` (runValues of a model) planned with `binary`; hardware is the caller's.
    static PlanJobInput input(const PlanningContext &context, const QVariantMap &values, const QString &binary,
                              bool isRun);
};

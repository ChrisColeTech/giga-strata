#pragma once

#include "gguf/model_layout.h"
#include "launch/run_environment.h"
#include "launch/run_settings.h"
#include "plan/plan_model.h"

#include <QString>
#include <QStringList>

#include <atomic>
#include <optional>

// The planning half of one Run, as llama_launcher.run_process does it before Popen.
struct RunPlan {
    QStringList logLines;              // every self.log message so far, verbatim (each ends in "\n")
    std::optional<ModelLayout> layout;
    std::optional<QString> draftPath;  // MTP draft to speculate with
    std::optional<QString> mmprojPath; // vision projector to load
    std::optional<Plan> plan;          // nullopt when the run failed
    qint64 reserveLastBytes = 0;       // draft + projector VRAM kept free on the last GPU
    QString hotPathToPrune;            // set once a hot file was named: Python prunes even if planning then fails
    QString error;                     // the failure text (the log already has its CRITICAL line)

    bool ok() const { return plan.has_value(); }
};

// run_process from resolve_server to the plan: find_draft, find_mmproj, the draft/projector
// reserve, per_expert_args and make_plan, with their log lines in the launcher's order. Pure:
// it reads through the environment's sources and writes nothing (02 D1, D2: the live preview
// and Run share this path).
class RunPlanner {
public:
    static RunPlan plan(const RunSettings &settings, const RunEnvironment &environment,
                        const std::atomic<bool> *cancel = nullptr);

    // run_process's failure line for `error`.
    static QString criticalLine(const QString &error);
};

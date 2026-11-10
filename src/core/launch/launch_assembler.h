#pragma once

#include "common/result.h"
#include "launch/run_planner.h"
#include "launch/run_settings.h"

#include <QProcessEnvironment>
#include <QSet>
#include <QString>
#include <QStringList>

// The process to start: argv (argv[0] is the binary), its environment, and the log lines
// run_process writes between the plan and the first server line.
struct AssembledLaunch {
    QStringList argv;
    QProcessEnvironment environment;
    QStringList logLines;  // verbatim, each ending in "\n"
};

// The Run half of run_process after planning: the file work planning deferred (hot file prune
// and write, 02 D1), set_usage_env with its .bak guard, and the command (CommandBuilder) with
// the CUDA_VISIBLE_DEVICES and "Executing command" lines. Shared by LaunchController and the
// golden runner so both run one path.
class LaunchAssembler {
public:
    // Prunes when a hot file was named (even after a failed plan); writes it unless reused.
    static Result<void> writeHotFile(const RunPlan &run, const QString &hotDirectory, const QString &modelPath);

    // `environment` is server_env's result. Fails only when the usage file cannot be moved aside;
    // the error is the run's failure text.
    static Result<AssembledLaunch> assemble(const RunSettings &settings, const RunPlan &run, const QString &binary,
                                            const QSet<QString> &flags, const QProcessEnvironment &environment,
                                            const QString &usageDirectory);
};

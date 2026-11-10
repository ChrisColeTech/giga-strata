#pragma once

#include "launch/launch_request.h"
#include "plan/plan_model.h"

#include <QList>
#include <QPair>
#include <QStringList>

struct LaunchCommand {
    QStringList argv;                              // argv[0] is the binary
    QList<QPair<QString, QString>> environment;    // tuning variables to add, in order
    QStringList logLines;                          // "[LAUNCHER] ..." lines run_process prints
};

// llama_launcher.run_process command assembly: plan args, network, alias, fit/load-mode, flash
// attention, per-architecture tuning, MTP speculation, vision projector, extra args.
class CommandBuilder {
public:
    static LaunchCommand build(const LaunchRequest &request, const Plan &plan);
};

#pragma once

#include "plan_job.h"

#include "common/result.h"
#include "launch/launch_assembler.h"

#include <QString>

class ServerBuild;

// After a Run plan: the file work it deferred (hot file prune/write, usage .bak guard) and the
// command, environment and launcher log lines, as run_process does before Popen.
class LaunchPreparer {
public:
    // `result` must hold parsed settings and a successful plan.
    static Result<AssembledLaunch> prepare(const PlanJobResult &result, const QString &binary,
                                           const ServerBuild &build, const QString &usageDirectory,
                                           const QString &hotDirectory);

    // http://<address>:<port> for clients: the first LAN IPv4 address when serving on the
    // network, else (or when there is none) 127.0.0.1.
    static QString clientUrl(bool isOnLan, const QString &port);
};

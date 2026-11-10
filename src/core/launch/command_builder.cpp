#include "launch/command_builder.h"

#include "launch/command_line.h"
#include "plan/tuning_tables.h"

namespace {

bool hasFlag(const LaunchRequest &request, const char *flag) {
    return request.flags.contains(QLatin1String(flag));
}

void appendMemoryMode(const LaunchRequest &request, const Plan &plan, QStringList &argv) {
    if (hasFlag(request, "--load-mode")) {
        argv << QStringLiteral("-lm") << (plan.useMmap ? QStringLiteral("mmap") : QStringLiteral("none"));
    } else if (!plan.useMmap) {
        argv << QStringLiteral("--no-mmap");
    }
}

void appendTuning(const LaunchRequest &request, const Plan &plan, LaunchCommand &command) {
    const auto tuning = TuningTables::tuningFor(plan.arch);
    if (!tuning) {
        return;
    }
    QStringList tuned = tuning->args;
    for (const auto &optional : tuning->argsIfSupported) {
        if (request.flags.contains(optional.first)) {
            tuned += optional.second;
        }
    }
    QString environmentText;
    for (const auto &variable : tuning->environment) {
        command.environment.append(variable);
        environmentText += QStringLiteral(" %1=%2").arg(variable.first, variable.second);
    }
    command.logLines.append(QStringLiteral("[LAUNCHER] %1 tuning: %2%3").arg(plan.arch, CommandLine::join(tuned), environmentText));
    command.argv += tuned;
}

void appendSpeculation(const LaunchRequest &request, const Plan &plan, QStringList &argv) {
    const auto speculation = TuningTables::speculationFor(plan.arch);
    if (!request.draftPath || !speculation) {
        return;
    }
    const bool isNgram = speculation->argsWithNgram && hasFlag(request, "ngram-mod");
    argv << QStringLiteral("-md") << *request.draftPath << QStringLiteral("-devd")
         << QStringLiteral("CUDA%1").arg(request.gpuCount - 1) << QStringLiteral("-ngld") << QStringLiteral("999");
    argv += isNgram ? *speculation->argsWithNgram : speculation->args;
}

} // namespace

LaunchCommand CommandBuilder::build(const LaunchRequest &request, const Plan &plan) {
    LaunchCommand command;
    if (!request.flags.isEmpty() && !hasFlag(request, "--override-tensor")) {
        command.logLines.append(QStringLiteral("[LAUNCHER] WARNING: this llama-server build has no --override-tensor; "
                                               "expert placement will not work. Use a newer llama.cpp build."));
    }
    QStringList &argv = command.argv;
    argv << request.binary << QStringLiteral("-m") << request.modelPath;
    argv += plan.args;
    argv << QStringLiteral("--host") << request.host << QStringLiteral("--port") << request.port;
    // API clients see the file name instead of the full path; -a in extra args overrides it
    const bool hasOwnAlias = request.extraArgs.contains(QStringLiteral("-a")) || request.extraArgs.contains(QStringLiteral("--alias"));
    if (hasFlag(request, "--alias") && !hasOwnAlias) {
        argv << QStringLiteral("--alias") << request.alias;
    }
    if (hasFlag(request, "--fit")) {
        argv << QStringLiteral("--fit") << QStringLiteral("off"); // keep llama.cpp from overriding the plan
    }
    appendMemoryMode(request, plan, argv);
    if (request.useFlashAttention) {
        argv << QStringLiteral("-fa") << QStringLiteral("on");
    }
    appendTuning(request, plan, command);
    appendSpeculation(request, plan, argv);
    if (request.mmprojPath) {
        argv << QStringLiteral("--mmproj") << *request.mmprojPath;
        if (hasFlag(request, "--mmproj-device")) {
            argv << QStringLiteral("--mmproj-device") << QStringLiteral("CUDA%1").arg(request.gpuCount - 1);
        }
    }
    argv += request.extraArgs;
    return command;
}

#include "launch/launch_assembler.h"

#include "common/model_names.h"
#include "launch/command_builder.h"
#include "launch/command_line.h"
#include "launch/launch_request.h"
#include "launch/usage_env.h"
#include "moe/hot_cache.h"

Result<void> LaunchAssembler::writeHotFile(const RunPlan &run, const QString &hotDirectory, const QString &modelPath) {
    if (run.hotPathToPrune.isEmpty()) {
        return Result<void>::success();
    }
    if (!run.plan || run.plan->isHotFileReused) {
        return HotCache::prune(hotDirectory, modelPath, run.hotPathToPrune);
    }
    return HotCache::writeAndPrune(hotDirectory, modelPath, run.plan->hotFilePath, run.plan->hotFileText);
}

Result<AssembledLaunch> LaunchAssembler::assemble(const RunSettings &settings, const RunPlan &run,
                                                  const QString &binary, const QSet<QString> &flags,
                                                  const QProcessEnvironment &environment,
                                                  const QString &usageDirectory) {
    AssembledLaunch launch;
    launch.environment = environment;
    if (settings.isRecordingUsage) {
        const QString usagePath = ModelNames::usagePath(usageDirectory, settings.modelPath);
        const auto recording = UsageEnv::prepare(*run.layout, flags, usageDirectory, usagePath, launch.environment);
        if (!recording.ok()) {
            return Result<AssembledLaunch>::failure(recording.error());
        }
        launch.logLines += recording.value().logLines;
    }
    LaunchRequest request;
    request.binary = binary;
    request.modelPath = settings.modelPath;
    request.host = settings.host;
    request.port = settings.port;
    request.extraArgs = settings.extraArgs;
    request.useFlashAttention = settings.useFlashAttention;
    request.gpuCount = int(settings.gpuIds.size());
    request.alias = ModelNames::modelStem(settings.modelPath);
    request.draftPath = run.draftPath;
    request.mmprojPath = run.mmprojPath;
    request.flags = flags;
    const LaunchCommand command = CommandBuilder::build(request, *run.plan);
    for (const QString &line : command.logLines) {
        launch.logLines.append(line + u'\n');
    }
    for (const auto &variable : command.environment) {
        launch.environment.insert(variable.first, variable.second);
    }
    launch.argv = command.argv;
    launch.logLines.append(QStringLiteral("[LAUNCHER] CUDA_VISIBLE_DEVICES=%1\n")
                               .arg(launch.environment.value(QStringLiteral("CUDA_VISIBLE_DEVICES"))));
    launch.logLines.append(QStringLiteral("[LAUNCHER] Executing command: %1\n\n").arg(CommandLine::join(launch.argv)));
    return Result<AssembledLaunch>::success(launch);
}

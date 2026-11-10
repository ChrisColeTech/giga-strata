#include "plan_job.h"

#include "plan_tiers.h"
#include "server_build.h"

#include "common/model_names.h"
#include "gguf/draft_finder.h"
#include "gguf/layout_reader.h"
#include "gguf/local_model_source.h"
#include "gguf/mmproj_finder.h"
#include "gguf/shard_finder.h"
#include "launch/run_settings_parser.h"
#include "moe/local_usage_source.h"
#include "moe/usage_io.h"

#include <algorithm>

namespace {

QVariantMap modelInfo(const PlanJobInput &input, const RunSettings &settings, const RunPlan &run,
                      const LocalModelSource &models) {
    QVariantMap info;
    std::optional<ModelLayout> layout = run.layout;
    if (!layout) {
        const auto read = LayoutReader::read(models, settings.modelPath);
        if (read.ok()) {
            layout = read.value();
        }
    }
    if (layout) {
        info.insert(QStringLiteral("type"), layout->arch);
        info.insert(QStringLiteral("layers"), layout->layerCount);
        info.insert(QStringLiteral("experts"), layout->expertCount);
    }
    const auto shards = ShardFinder::shardPaths(models, settings.modelPath);
    if (shards.ok()) {
        qint64 size = 0;
        for (const QString &shard : shards.value()) {
            size += models.fileSize(shard).value_or(0);
        }
        info.insert(QStringLiteral("sizeBytes"), double(size));
        info.insert(QStringLiteral("files"), int(shards.value().size()));
    }
    info.insert(QStringLiteral("draftPath"),
                DraftFinder::find(models, settings.modelPath, {input.modelsDirectory}).value_or(QString()));
    info.insert(QStringLiteral("projectorPath"),
                MmprojFinder::find(models, settings.modelPath, {input.modelsDirectory}).value_or(QString()));
    const QString usagePath = ModelNames::usagePath(input.usageDirectory, settings.modelPath);
    const LocalUsageSource usage;
    const auto tokens = usage.exists(usagePath) ? UsageIo::usageTokens(usage, {usagePath})
                                                : Result<QPair<qint64, qint64>>::success({0, 0});
    info.insert(QStringLiteral("decodeTokens"), double(tokens.ok() ? tokens.value().first : 0));
    info.insert(QStringLiteral("promptTokens"), double(tokens.ok() ? tokens.value().second : 0));
    return info;
}

QVector<Gpu> selectedGpus(const QVector<int> &ids, const HardwareSnapshot &hardware) {
    QVector<Gpu> gpus;
    for (const int id : ids) {
        const auto found = std::find_if(hardware.gpus.cbegin(), hardware.gpus.cend(),
                                        [id](const Gpu &gpu) { return gpu.index == id; });
        if (found != hardware.gpus.cend()) {
            gpus.append(*found);
        }
    }
    return gpus;
}

} // namespace

PlanJobInput PlanJob::input(const PlanningContext &context, const QVariantMap &values, const QString &binary,
                            bool isRun) {
    PlanJobInput input;
    input.values = values;
    input.binary = binary;
    input.modelsDirectory = context.modelsDirectory;
    input.usageDirectory = context.usageDirectory;
    input.hotDirectory = context.hotDirectory;
    input.isRun = isRun;
    input.build = context.build;
    input.shards = context.shards;
    return input;
}

PlanJobResult PlanJob::run(const PlanJobInput &input) {
    PlanJobResult result;
    const auto settings = RunSettingsParser::parse(input.values);
    if (!settings.ok()) {
        result.error = settings.error();
        return result;
    }
    result.settings = settings.value();
    result.flags = input.isRun ? input.build->flagsFor(input.binary) : input.build->planningFlagsFor(input.binary);
    const LocalModelSource models(input.shards);
    const LocalUsageSource usage;
    RunEnvironment environment;
    environment.models = &models;
    environment.usage = &usage;
    environment.hardware = input.hardware;
    environment.flags = result.flags;
    environment.modelsDirectory = input.modelsDirectory;
    environment.usageDirectory = input.usageDirectory;
    environment.hotDirectory = input.hotDirectory;
    result.run = RunPlanner::plan(settings.value(), environment, input.cancel.get());
    result.error = result.run.error;
    if (input.isRun) {
        return result;
    }
    result.modelInfo = modelInfo(input, settings.value(), result.run, models);
    if (result.run.ok()) {
        PlanTiers::Input view;
        view.layout = &*result.run.layout;
        view.plan = &*result.run.plan;
        view.gpus = selectedGpus(settings.value().gpuIds, input.hardware);
        view.contextSize = settings.value().contextSize;
        view.reserveLastBytes = result.run.reserveLastBytes;
        view.freeRamBytes = input.hardware.freeRamBytes;
        result.tiers = PlanTiers::tiers(view);
        result.summary = PlanTiers::summary(view);
    }
    return result;
}

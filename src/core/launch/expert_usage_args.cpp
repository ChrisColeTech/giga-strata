#include "launch/expert_usage_args.h"

#include "common/constants.h"
#include "common/model_names.h"
#include "common/python_format.h"
#include "gguf/model_source.h"
#include "moe/hot_cache.h"
#include "moe/usage_io.h"
#include "moe/usage_source.h"

namespace {

std::optional<FileStamp> modelStamp(const ModelSource &models, const QString &path) {
    const auto size = models.fileSize(path);
    const auto modified = models.modifiedTimeNs(path);
    if (!size || !modified) {
        return std::nullopt;
    }
    return FileStamp{path, *size, *modified};
}

std::optional<FileStamp> usageStamp(const UsageSource &usage, const QString &path) {
    const auto size = usage.fileSize(path);
    const auto modified = usage.modifiedTimeNs(path);
    if (!size || !modified) {
        return std::nullopt;
    }
    return FileStamp{path, *size, *modified};
}

QString cannotStat(const QString &path) {
    return QStringLiteral("Cannot read %1.").arg(path);
}

} // namespace

Result<ExpertUsagePlan> ExpertUsageArgs::decide(const RunSettings &settings, const RunEnvironment &environment,
                                                qint64 reserveBytes) {
    ExpertUsagePlan plan;
    if (!settings.usePerExpert) {
        return Result<ExpertUsagePlan>::success(plan);
    }
    const QString usagePath = ModelNames::usagePath(environment.usageDirectory, settings.modelPath);
    QStringList files;
    if (environment.usage->exists(usagePath)) {
        files.append(usagePath);
    }
    // usage_summary: an unreadable profile counts as no profile
    const auto tokens = UsageIo::usageTokens(*environment.usage, files);
    if (files.isEmpty() || !tokens.ok()) {
        plan.logLines.append(QStringLiteral("[MOE] Per-expert placement is on, but there is no recorded usage for this "
                                            "model; placing whole layers\n"));
        return Result<ExpertUsagePlan>::success(plan);
    }
    if (!environment.flags.contains(QStringLiteral("--moe-hot"))) {
        plan.logLines.append(QStringLiteral("[MOE] This llama-server build has no --moe-hot; placing whole layers\n"));
        return Result<ExpertUsagePlan>::success(plan);
    }
    const qint64 decode = tokens.value().first;
    plan.logLines.append(QStringLiteral("[MOE] Recorded usage: %1 generated + %2 prompt tokens in %3\n")
                             .arg(python_format::thousands(decode), python_format::thousands(tokens.value().second),
                                  files.constFirst()));
    if (decode < constants::USAGE_MIN_TOKENS) {
        plan.logLines.append(QStringLiteral("[MOE] WARNING: under %1 generated tokens recorded; the profile may not "
                                            "match your workload and can make generation slower\n")
                                 .arg(python_format::thousands(constants::USAGE_MIN_TOKENS)));
    }
    HotCacheKey key;
    const auto model = modelStamp(*environment.models, settings.modelPath);
    if (!model) {
        return Result<ExpertUsagePlan>::failure(cannotStat(settings.modelPath));
    }
    key.model = *model;
    for (const QString &file : files) {
        const auto stamp = usageStamp(*environment.usage, file);
        if (!stamp) {
            return Result<ExpertUsagePlan>::failure(cannotStat(file));
        }
        key.usageFiles.append(*stamp);
    }
    key.gpuIds = settings.gpuIds;
    key.contextSize = settings.contextSize;
    key.marginMib = settings.marginMib;
    key.microBatch = settings.microBatch;
    key.reserveBytes = reserveBytes;

    PerExpertRequest request;
    request.counts = UsageIo::readCounts(*environment.usage, files);
    request.hotFilePath = HotCache::filePath(environment.hotDirectory, key);
    request.isCostAware = true;  // always cost mode: it sizes the first GPU's reserve for split layers
    if (environment.usage->exists(request.hotFilePath)) {
        request.cachedHotFile = environment.usage->readAll(request.hotFilePath);
    }
    plan.request = request;
    return Result<ExpertUsagePlan>::success(plan);
}

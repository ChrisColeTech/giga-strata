#include "launch/run_settings_parser.h"

#include "common/python_int.h"
#include "common/python_text.h"
#include "launch/extra_args.h"

#include <limits>

namespace {

QString text(const QVariantMap &values, const char *key) {
    return values.value(QLatin1String(key)).toString();
}

bool flag(const QVariantMap &values, const char *key, bool fallback) {
    const QVariant value = values.value(QLatin1String(key));
    return value.isValid() ? value.toBool() : fallback;
}

std::optional<int> smallInt(const QString &value) {
    const auto parsed = python_int::parse(value);
    if (!parsed || *parsed < std::numeric_limits<int>::min() || *parsed > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }
    return int(*parsed);
}

// [int(x) for x in gpus.replace(" ", "").split(",") if x]
std::optional<QVector<int>> gpuIds(const QString &gpus) {
    QVector<int> ids;
    for (const QString &item : QString(gpus).remove(u' ').split(u',', Qt::SkipEmptyParts)) {
        const auto id = smallInt(item);
        if (!id) {
            return std::nullopt;
        }
        ids.append(*id);
    }
    return ids;
}

// str.strip('"'): only double quotes, from both ends.
QString stripQuotes(QString value) {
    while (value.startsWith(u'"')) {
        value.remove(0, 1);
    }
    while (value.endsWith(u'"')) {
        value.chop(1);
    }
    return value;
}

} // namespace

Result<RunSettings> RunSettingsParser::parse(const QVariantMap &values) {
    RunSettings settings;
    settings.modelPath = python_text::strip(text(values, "model"));
    if (text(values, "model").isEmpty()) {
        return Result<RunSettings>::failure(missingModelMessage());
    }
    const auto ids = gpuIds(text(values, "gpus"));
    const auto context = python_int::parse(text(values, "ctx"));
    const auto margin = python_int::parse(text(values, "margin"));
    const QString microBatch = python_text::strip(text(values, "ubatch")).toLower();
    const bool isAutoBatch = microBatch.isEmpty() || microBatch == QLatin1String("auto");
    const auto fixedBatch = isAutoBatch ? std::optional<int>() : smallInt(text(values, "ubatch"));
    const auto extra = ExtraArgs::split(text(values, "extra"));
    if (!ids || !context || !margin || (!isAutoBatch && !fixedBatch) || !extra.ok()) {
        return Result<RunSettings>::failure(notWholeNumbersMessage());
    }
    settings.binary = python_text::strip(text(values, "binary"));
    settings.gpuIds = *ids;
    settings.contextSize = *context;
    settings.marginMib = *margin;
    settings.microBatch = fixedBatch;
    settings.host = python_text::strip(text(values, "host"));
    settings.port = python_text::strip(text(values, "port"));
    settings.useMmap = flag(values, "mmap", true);
    settings.useFlashAttention = flag(values, "fa", true);
    settings.extraArgs = extra.value();
    settings.autoServer = flag(values, "auto_server", true);
    settings.useMtp = flag(values, "mtp", true);
    settings.useVision = flag(values, "vision", false);
    settings.mmprojPath = stripQuotes(python_text::strip(text(values, "mmproj")));
    settings.isRecordingUsage = flag(values, "record_usage", false);
    settings.usePerExpert = flag(values, "per_expert", false);
    return Result<RunSettings>::success(settings);
}

QString RunSettingsParser::missingModelMessage() {
    return QStringLiteral("Please select your primary model shard (.gguf) file first!");
}

QString RunSettingsParser::notWholeNumbersMessage() {
    return QStringLiteral("GPU IDs, context size, margin and micro-batch must be whole numbers "
                          "(micro-batch may also be 'auto').");
}

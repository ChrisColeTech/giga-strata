#include "moe/usage_io.h"

#include "common/python_json.h"
#include "moe/usage_source.h"

#include <QJsonArray>
#include <QJsonObject>

#include <algorithm>
#include <cmath>

namespace {

// Python's exact wording: read_counts errors reach the Run log verbatim as "<path>: not a ...".
const QString NOT_USAGE = QStringLiteral("not a llama-moe-usage file");

bool isJsonPath(const QString &path) {
    return path.toLower().endsWith(QStringLiteral(".json"));
}

// Python float(x) for the JSON scalars a usage file can hold.
std::optional<double> pythonFloat(const QJsonValue &value) {
    if (value.isBool()) {
        return value.toBool() ? 1.0 : 0.0;
    }
    if (value.isDouble()) {
        return value.toDouble();
    }
    return std::nullopt;
}

// Python int(x): floats truncate toward zero.
std::optional<qint64> pythonInt(const QJsonValue &value) {
    const auto number = pythonFloat(value);
    if (!number || !std::isfinite(*number) || std::abs(*number) >= 9.2e18) {
        return std::nullopt;
    }
    if (value.isDouble() && value.toInteger(-1) == value.toDouble()) {
        return value.toInteger();
    }
    return qint64(std::trunc(*number));
}

// Python truthiness of one counts entry; `if c` skips null, [] and other falsy values.
bool isTruthy(const QJsonValue &value) {
    if (value.isArray()) {
        return !value.toArray().isEmpty();
    }
    if (value.isObject()) {
        return !value.toObject().isEmpty();
    }
    if (value.isString()) {
        return !value.toString().isEmpty();
    }
    if (value.isBool()) {
        return value.toBool();
    }
    return value.isDouble() && value.toDouble() != 0.0;
}

QString cannotRead(const QString &path) {
    return QStringLiteral("Cannot read the recorded expert usage in %1.").arg(path);
}

} // namespace

Result<UsageCounts> UsageIo::readUsage(const QByteArray &json, std::optional<QString> bucket) {
    const auto document = python_json::parse(json);
    if (!document || !document->isObject()) {
        return Result<UsageCounts>::failure(NOT_USAGE);
    }
    const QJsonObject root = document->object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("llama-moe-usage")) {
        return Result<UsageCounts>::failure(NOT_USAGE);
    }
    if (!bucket) {
        const QJsonObject tokens = root.value(QStringLiteral("tokens")).toObject();
        const auto decode = pythonFloat(tokens.value(QStringLiteral("decode")));
        if (!decode) {
            return Result<UsageCounts>::failure(NOT_USAGE);
        }
        bucket = *decode > 0 ? QStringLiteral("decode") : QStringLiteral("batch");
    }
    const QJsonValue layers = root.value(QStringLiteral("counts")).toObject().value(*bucket);
    if (!layers.isArray()) {
        return Result<UsageCounts>::failure(NOT_USAGE);
    }
    UsageCounts counts;
    const QJsonArray layerArray = layers.toArray();
    for (qsizetype layer = 0; layer < layerArray.size(); ++layer) {
        const QJsonValue entry = layerArray.at(layer);
        if (!isTruthy(entry)) {
            continue;
        }
        if (!entry.isArray()) {
            return Result<UsageCounts>::failure(NOT_USAGE);
        }
        QVector<double> perExpert;
        for (const QJsonValue &count : entry.toArray()) {
            const auto value = pythonFloat(count);
            if (!value) {
                return Result<UsageCounts>::failure(NOT_USAGE);
            }
            perExpert.append(*value);
        }
        counts.insert(int(layer), perExpert);
    }
    return Result<UsageCounts>::success(counts);
}

Result<QPair<qint64, qint64>> UsageIo::usageTokens(const UsageSource &source, const QStringList &paths) {
    qint64 decode = 0;
    qint64 batch = 0;
    for (const QString &path : paths) {
        if (!isJsonPath(path)) {
            continue;
        }
        const auto bytes = source.readAll(path);
        if (!bytes) {
            return Result<QPair<qint64, qint64>>::failure(cannotRead(path));
        }
        const auto document = python_json::parse(*bytes);
        if (!document || !document->isObject()) {
            return Result<QPair<qint64, qint64>>::failure(cannotRead(path));
        }
        const QJsonValue tokens = document->object().value(QStringLiteral("tokens"));
        if (!tokens.isUndefined() && !tokens.isObject()) {
            return Result<QPair<qint64, qint64>>::failure(cannotRead(path));
        }
        const QJsonObject tokenObject = tokens.toObject();
        const auto fileDecode = pythonInt(tokenObject.value(QStringLiteral("decode")).isUndefined()
                                              ? QJsonValue(0)
                                              : tokenObject.value(QStringLiteral("decode")));
        const auto fileBatch = pythonInt(tokenObject.value(QStringLiteral("batch")).isUndefined()
                                             ? QJsonValue(0)
                                             : tokenObject.value(QStringLiteral("batch")));
        if (!fileDecode || !fileBatch) {
            return Result<QPair<qint64, qint64>>::failure(cannotRead(path));
        }
        decode += *fileDecode;
        batch += *fileBatch;
    }
    return Result<QPair<qint64, qint64>>::success({decode, batch});
}

Result<UsageCounts> UsageIo::readCounts(const UsageSource &source, const QStringList &paths) {
    UsageCounts summed;
    for (const QString &path : paths) {
        if (!isJsonPath(path)) {
            return Result<UsageCounts>::failure(
                QStringLiteral("Expert usage can only be read from llama-server usage files (.json); %1 is not one.")
                    .arg(path));
        }
        const auto bytes = source.readAll(path);
        if (!bytes) {
            return Result<UsageCounts>::failure(cannotRead(path));
        }
        const auto file = readUsage(*bytes);
        if (!file.ok()) {
            return Result<UsageCounts>::failure(QStringLiteral("%1: %2").arg(path, file.error()));
        }
        for (auto layer = file.value().cbegin(); layer != file.value().cend(); ++layer) {
            if (!summed.contains(layer.key())) {
                summed.insert(layer.key(), layer.value());
                continue;
            }
            const QVector<double> previous = summed.value(layer.key());
            QVector<double> added(std::min(previous.size(), layer.value().size()));
            for (qsizetype expert = 0; expert < added.size(); ++expert) {
                added[expert] = previous[expert] + layer.value()[expert];
            }
            summed.insert(layer.key(), added);
        }
    }
    return Result<UsageCounts>::success(summed);
}

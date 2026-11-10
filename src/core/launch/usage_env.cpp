#include "launch/usage_env.h"

#include "common/constants.h"
#include "common/python_json.h"
#include "gguf/model_layout.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>

#include <windows.h>

namespace {

const QString USAGE_OUT = QStringLiteral("LLAMA_MOE_USAGE_OUT");
const QString USAGE_FLUSH = QStringLiteral("LLAMA_MOE_USAGE_FLUSH");

// Python == between a JSON value and an int: numbers compare by value, true/false are 1/0.
bool equalsInt(const QJsonValue &value, qint64 expected) {
    if (value.isBool()) {
        return (value.toBool() ? 1 : 0) == expected;
    }
    return value.isDouble() && value.toDouble() == double(expected);
}

// os.replace: the server cannot do this move itself on Windows while it holds the file open.
bool replaceFile(const QString &from, const QString &to) {
    const std::wstring source = QDir::toNativeSeparators(from).toStdWString();
    const std::wstring target = QDir::toNativeSeparators(to).toStdWString();
    return MoveFileExW(source.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE;
}

} // namespace

bool UsageEnv::isCompatible(bool exists, const std::optional<QByteArray> &contents, const ModelLayout &layout) {
    if (!exists) {
        return true;
    }
    if (!contents) {
        return false;
    }
    const auto document = python_json::parse(*contents);
    if (!document || !document->isObject()) {
        return false;
    }
    const QJsonObject recorded = document->object();
    const QJsonValue arch = recorded.value(QStringLiteral("arch"));
    return arch.isString() && arch.toString() == layout.arch
           && equalsInt(recorded.value(QStringLiteral("n_expert")), layout.expertCount)
           && equalsInt(recorded.value(QStringLiteral("n_layer")), layout.layerCount + layout.skippedLayerCount);
}

UsageRecording UsageEnv::plan(const ModelLayout &layout, const QSet<QString> &flags, const QString &usagePath,
                              bool isCompatible, const QProcessEnvironment &environment) {
    UsageRecording recording;
    if (layout.expertCount == 0) {
        recording.logLines.append(QStringLiteral("[MOE] Not recording expert usage: not a MoE model\n"));
        return recording;
    }
    recording.isRecording = true;
    recording.usagePath = usagePath;
    if (!flags.isEmpty() && !flags.contains(QStringLiteral("--moe-hot"))) {
        recording.logLines.append(QStringLiteral("[MOE] WARNING: this llama-server build predates expert usage "
                                                 "recording; nothing may be written\n"));
    }
    if (!isCompatible) {
        recording.needsBackup = true;
        recording.logLines.append(QStringLiteral("[MOE] %1 was not recorded with this model; moved it to %1.bak\n")
                                      .arg(usagePath));
    }
    recording.flushTokens = environment.contains(USAGE_FLUSH) ? environment.value(USAGE_FLUSH)
                                                              : QString::number(constants::USAGE_FLUSH_TOKENS);
    recording.logLines.append(QStringLiteral("[MOE] Recording expert usage to %1 (every %2 tokens)\n")
                                  .arg(usagePath, recording.flushTokens));
    return recording;
}

void UsageEnv::apply(const UsageRecording &recording, QProcessEnvironment &environment) {
    if (!recording.isRecording) {
        return;
    }
    environment.insert(USAGE_OUT, recording.usagePath);
    environment.insert(USAGE_FLUSH, recording.flushTokens);
}

Result<UsageRecording> UsageEnv::prepare(const ModelLayout &layout, const QSet<QString> &flags,
                                         const QString &usageDirectory, const QString &usagePath,
                                         QProcessEnvironment &environment) {
    if (layout.expertCount != 0 && !QDir().mkpath(usageDirectory)) {
        return Result<UsageRecording>::failure(
            QStringLiteral("Cannot create the expert usage folder %1.").arg(usageDirectory));
    }
    const QFileInfo info(usagePath);
    std::optional<QByteArray> contents;
    QFile file(usagePath);
    if (info.exists() && file.open(QIODevice::ReadOnly)) {
        contents = file.readAll();
        file.close();
    }
    const bool isCompatibleFile = layout.expertCount == 0 || isCompatible(info.exists(), contents, layout);
    const UsageRecording recording = plan(layout, flags, usagePath, isCompatibleFile, environment);
    if (recording.needsBackup && !replaceFile(usagePath, usagePath + QStringLiteral(".bak"))) {
        return Result<UsageRecording>::failure(
            QStringLiteral("Cannot move %1 aside: it was recorded with another model and is in use.").arg(usagePath));
    }
    apply(recording, environment);
    return Result<UsageRecording>::success(recording);
}

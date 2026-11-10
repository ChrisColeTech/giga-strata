#include "moe/local_usage_source.h"

#include "gguf/local_model_source.h"

#include <QFile>
#include <QFileInfo>

bool LocalUsageSource::exists(const QString &path) const {
    return QFileInfo::exists(path);
}

std::optional<QByteArray> LocalUsageSource::readAll(const QString &path) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    return file.readAll();
}

std::optional<qint64> LocalUsageSource::fileSize(const QString &path) const {
    return LocalModelSource().fileSize(path);
}

std::optional<qint64> LocalUsageSource::modifiedTimeNs(const QString &path) const {
    // One implementation of the NTFS stamp conversion (R15): the model source already has it.
    return LocalModelSource().modifiedTimeNs(path);
}

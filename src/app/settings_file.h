#pragma once

#include "common/result.h"

#include <QByteArray>
#include <QJsonObject>
#include <QStringList>

// launcher_settings.json on disk: tolerant read, ordered atomic write. QJsonObject sorts keys,
// so the writer puts `leadingKeys` first; that keeps the file in the order the Python launcher
// wrote it, which makes it diffable during the transition (02 H12).
class SettingsFile {
public:
    // A missing file is not an error: it reads as an empty object (first launch).
    static Result<QJsonObject> read(const QString &filePath);
    static Result<void> write(const QString &filePath, const QJsonObject &document, const QStringList &leadingKeys);
    static QByteArray serialize(const QJsonObject &document, const QStringList &leadingKeys);
};

#include "moe/hot_cache.h"

#include "common/model_names.h"
#include "common/path_key.h"
#include "common/python_format.h"
#include "common/python_json.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>

namespace {

QString stampList(const QVector<FileStamp> &files) {
    QStringList parts;
    for (const FileStamp &file : files) {
        // Python writes the tuple (normcase(f), st_mtime_ns, size): mtime before size.
        parts.append(u'[' + python_json::quote(ModelNames::normcase(file.path)) + QStringLiteral(", ")
                     + QString::number(file.modifiedTimeNs) + QStringLiteral(", ") + QString::number(file.sizeBytes)
                     + u']');
    }
    return u'[' + parts.join(QStringLiteral(", ")) + u']';
}

} // namespace

QString HotCache::keyJson(const HotCacheKey &key) {
    const QString modelKey = ModelNames::modelKey(key.model.path, key.model.sizeBytes, key.model.modifiedTimeNs);
    const QString microBatch =
        key.microBatch ? QString::number(*key.microBatch) : python_json::quote(QStringLiteral("auto"));
    const QStringList parts{python_json::quote(modelKey),       stampList(key.usageFiles),
                            python_format::intList(key.gpuIds), QString::number(key.contextSize),
                            QString::number(key.marginMib),     microBatch,
                            QString::number(key.reserveBytes)};
    return u'[' + parts.join(QStringLiteral(", ")) + u']';
}

QString HotCache::fileName(const HotCacheKey &key) {
    const QByteArray digest = QCryptographicHash::hash(keyJson(key).toUtf8(), QCryptographicHash::Sha1).toHex();
    return ModelNames::modelStem(key.model.path) + u'-' + QString::fromLatin1(digest.left(12))
           + QStringLiteral(".moe-hot.txt");
}

QString HotCache::filePath(const QString &hotDirectory, const HotCacheKey &key) {
    return path_key::join(hotDirectory, fileName(key));
}

QStringList HotCache::staleFiles(const QString &hotDirectory, const QStringList &existingNames,
                                 const QString &modelPath, const QString &currentPath) {
    const QRegularExpression ownName(QRegularExpression::anchoredPattern(
        QRegularExpression::escape(ModelNames::modelStem(modelPath))
        + QStringLiteral("-[0-9a-f]{12}\\.moe-hot\\.txt")));
    QStringList stale;
    for (const QString &name : existingNames) {
        const QString path = path_key::join(hotDirectory, name);
        if (ownName.match(name).hasMatch() && ModelNames::normcase(path) != ModelNames::normcase(currentPath)) {
            stale.append(path);
        }
    }
    return stale;
}

Result<void> HotCache::prune(const QString &hotDirectory, const QString &modelPath, const QString &currentPath) {
    if (!QDir().mkpath(hotDirectory)) {
        return Result<void>::failure(QStringLiteral("Cannot create the hot expert folder %1.").arg(hotDirectory));
    }
    const QStringList names = QDir(hotDirectory).entryList(QDir::AllEntries | QDir::Hidden | QDir::System |
                                                           QDir::NoDotAndDotDot);
    for (const QString &stale : staleFiles(hotDirectory, names, modelPath, currentPath)) {
        QFile::remove(stale);
    }
    return Result<void>::success();
}

Result<void> HotCache::writeAndPrune(const QString &hotDirectory, const QString &modelPath,
                                     const QString &currentPath, const QByteArray &text) {
    const Result<void> pruned = prune(hotDirectory, modelPath, currentPath);
    if (!pruned.ok()) {
        return pruned;
    }
    QSaveFile file(currentPath);
    if (!file.open(QIODevice::WriteOnly) || file.write(text) != text.size() || !file.commit()) {
        return Result<void>::failure(QStringLiteral("Cannot write the hot expert file %1.").arg(currentPath));
    }
    return Result<void>::success();
}

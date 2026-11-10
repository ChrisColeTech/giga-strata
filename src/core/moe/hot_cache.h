#pragma once

#include "common/result.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

// One file as os.stat saw it when the cache key was built.
struct FileStamp {
    QString path;
    qint64 sizeBytes = 0;
    qint64 modifiedTimeNs = 0;
};

// Everything llama_launcher.hot_cache_path hashes: a hot file is valid for exactly this model,
// usage, GPU set, context, margin, micro-batch and last-GPU reserve.
struct HotCacheKey {
    FileStamp model;
    QVector<FileStamp> usageFiles;
    QVector<int> gpuIds;
    qint64 contextSize = 0;
    qint64 marginMib = 0;
    std::optional<int> microBatch;  // nullopt is the setting "auto"
    qint64 reserveBytes = 0;
};

// Naming, pruning and writing of cached --moe-hot files (hot_cache_path). The name must stay
// exactly what Python produced or every existing hot file is orphaned (02 H9). Planning only
// computes names; files are written and stale ones removed on Run (02 D1).
class HotCache {
public:
    // The exact json.dumps text that is hashed.
    static QString keyJson(const HotCacheKey &key);

    // "<model stem>-<first 12 hex of sha1(keyJson)>.moe-hot.txt"
    static QString fileName(const HotCacheKey &key);

    // os.path.join(hotDirectory, fileName). The directory is used as given, never made absolute,
    // so a relative hot directory gives a relative path just as in Python.
    static QString filePath(const QString &hotDirectory, const HotCacheKey &key);

    // Older hot files of the same model in `existingNames` (the directory's entries): exactly
    // "<stem>-<12 lowercase hex>.moe-hot.txt", case-sensitive like Python's re.fullmatch, and not
    // `currentPath` (compared after normcase). Returned as paths joined onto hotDirectory.
    static QStringList staleFiles(const QString &hotDirectory, const QStringList &existingNames,
                                  const QString &modelPath, const QString &currentPath);

    // Run-time I/O, the side effect of hot_cache_path: create the directory and remove the stale
    // files (failures ignored, as Python does). Due whenever a hot file was named, even when the
    // cached file is reused or planning then fails.
    static Result<void> prune(const QString &hotDirectory, const QString &modelPath, const QString &currentPath);

    // prune, then write `text` to currentPath (make_plan's write_hot).
    static Result<void> writeAndPrune(const QString &hotDirectory, const QString &modelPath,
                                      const QString &currentPath, const QByteArray &text);
};

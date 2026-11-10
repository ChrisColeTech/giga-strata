#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <optional>

class ShardCache;

// Every filesystem read the GGUF and planning code makes goes through this, so golden tests can
// serve compressed header fixtures in place of 100 GB model files (02 section 5, R31).
// Paths are passed and returned exactly as the caller spelled them; normalization is the
// caller's job, matching the Python reference.
class ModelSource {
public:
    virtual ~ModelSource() = default;

    virtual bool exists(const QString &path) const = 0;
    virtual bool isDirectory(const QString &path) const = 0;
    // Whole-file size in bytes; the GGUF header alone cannot tell where the last tensor ends.
    virtual std::optional<qint64> fileSize(const QString &path) const = 0;
    // os.stat st_mtime_ns: model_key truncates it to seconds, hot-cache keys use it whole.
    virtual std::optional<qint64> modifiedTimeNs(const QString &path) const = 0;
    // At least the first `maxBytes` bytes of the file (fewer if the file is shorter).
    virtual std::optional<QByteArray> readPrefix(const QString &path, qint64 maxBytes) const = 0;
    // Files under `directory` matching a glob `pattern` (e.g. "*mmproj*.gguf"), optionally
    // recursive, sorted the way Python's sorted(glob.glob(...)) sorts them.
    virtual QStringList listFiles(const QString &directory, const QString &pattern, bool isRecursive) const = 0;
    // Where GgufReader keeps parsed headers of this source's files; none by default.
    virtual ShardCache *shardCache() const { return nullptr; }
};

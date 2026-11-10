#pragma once

#include <QString>

// The names llama_launcher.py derives from a model path. Recorded usage files, hot files and
// remembered server picks are found by these strings, so they must not change (00 section 7).
class ModelNames {
public:
    // model_key: "normcase(abspath(model))|st_size|int(st_mtime)". st_mtime is Python's float
    // seconds, built from the nanosecond stamp the way os.stat builds it (02 H10).
    static QString modelKey(const QString &modelPath, qint64 sizeBytes, qint64 modifiedTimeNs);

    // model_stem: the file name without ".gguf" and the "-0000N-of-0000M" split suffix.
    static QString modelStem(const QString &modelPath);

    // usage_path: <usage directory>\<stem>.usage.json
    static QString usagePath(const QString &usageDirectory, const QString &modelPath);

    // os.path.basename on Windows: after the last slash or backslash, drive letter dropped.
    static QString baseName(const QString &path);

    // os.path.normcase on Windows: lowercase, forward slashes turned into backslashes. No
    // abspath, which is why the hot-cache key cannot use path_key::of for usage files.
    static QString normcase(const QString &path);

    // int(st_mtime) for a stamp in nanoseconds since the epoch.
    static qint64 pythonMtimeSeconds(qint64 modifiedTimeNs);
};

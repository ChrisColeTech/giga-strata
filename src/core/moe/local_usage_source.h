#pragma once

#include "moe/usage_source.h"

// UsageSource over the real filesystem.
class LocalUsageSource : public UsageSource {
public:
    bool exists(const QString &path) const override;
    std::optional<QByteArray> readAll(const QString &path) const override;
    std::optional<qint64> fileSize(const QString &path) const override;
    // The raw NTFS write time as Python's st_mtime_ns reports it (hot-cache keys, 02 H9).
    std::optional<qint64> modifiedTimeNs(const QString &path) const override;
};

#pragma once

#include "gguf/model_source.h"

// ModelSource over the real filesystem.
class LocalModelSource : public ModelSource {
public:
    // `cache` (optional, not owned) keeps parsed headers between reads (02 D4).
    explicit LocalModelSource(ShardCache *cache = nullptr) : m_cache(cache) {}

    bool exists(const QString &path) const override;
    bool isDirectory(const QString &path) const override;
    std::optional<qint64> fileSize(const QString &path) const override;
    std::optional<qint64> modifiedTimeNs(const QString &path) const override;
    std::optional<QByteArray> readPrefix(const QString &path, qint64 maxBytes) const override;
    QStringList listFiles(const QString &directory, const QString &pattern, bool isRecursive) const override;
    ShardCache *shardCache() const override { return m_cache; }

private:
    ShardCache *m_cache;
};

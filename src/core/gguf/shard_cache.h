#pragma once

#include "gguf/gguf_shard.h"

#include <QHash>
#include <QMutex>
#include <QQueue>
#include <QString>

#include <optional>

// Parsed GGUF headers, keyed by path, size and modification time so an edited file is read
// again (02 D4). A planning run reads the model's header from the layout, draft and projector
// searches; GLM's 11 MB tokenizer table would otherwise be parsed several times per re-plan.
// Bounded, oldest first out; safe to share between worker threads.
class ShardCache {
public:
    static constexpr int DEFAULT_CAPACITY = 16;

    explicit ShardCache(int capacity = DEFAULT_CAPACITY) : m_capacity(capacity) {}

    static QString keyFor(const QString &path, qint64 sizeBytes, qint64 modifiedTimeNs);

    std::optional<GgufShard> find(const QString &key) const;
    void insert(const QString &key, const GgufShard &shard);

private:
    int m_capacity;
    mutable QMutex m_mutex;
    QHash<QString, GgufShard> m_shards;
    QQueue<QString> m_order;
};

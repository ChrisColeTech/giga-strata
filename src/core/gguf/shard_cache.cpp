#include "gguf/shard_cache.h"

#include <QMutexLocker>

QString ShardCache::keyFor(const QString &path, qint64 sizeBytes, qint64 modifiedTimeNs) {
    return path + u'|' + QString::number(sizeBytes) + u'|' + QString::number(modifiedTimeNs);
}

std::optional<GgufShard> ShardCache::find(const QString &key) const {
    const QMutexLocker locker(&m_mutex);
    const auto found = m_shards.constFind(key);
    return found == m_shards.cend() ? std::nullopt : std::optional<GgufShard>(*found);
}

void ShardCache::insert(const QString &key, const GgufShard &shard) {
    const QMutexLocker locker(&m_mutex);
    if (m_shards.contains(key)) {
        return;
    }
    while (!m_order.isEmpty() && m_order.size() >= m_capacity) {
        m_shards.remove(m_order.dequeue());
    }
    m_shards.insert(key, shard);
    m_order.enqueue(key);
}

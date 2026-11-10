#pragma once

#include <QDateTime>
#include <QString>

// One row of the Models table (05 section 1 roles).
struct ModelEntry {
    QString name;          // model stem
    QString path;          // first shard
    QString type;          // architecture
    QString quant;
    qint64 sizeBytes = 0;  // all shards
    int experts = 0;       // 0 for dense
    int files = 1;         // shards
    QDateTime modified;    // first shard
    bool hasDraft = false;
    bool hasProjector = false;
    bool hasUsage = false;
};

#pragma once

#include <QMap>
#include <QString>
#include <QVariantMap>

// strata_planner._read_shard result for one GGUF file. Metadata values follow the Python
// types: integers as qlonglong/qulonglong, floats as double, bool, QString, QVariantList for
// arrays, and a null QVariant for large string arrays that were skipped undecoded.
struct GgufShard {
    QVariantMap metadata;
    QMap<QString, qint64> tensorSizes; // bytes, from offset gaps; the last tensor runs to file end
};

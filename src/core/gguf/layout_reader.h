#pragma once

#include "common/result.h"
#include "gguf/gguf_shard.h"
#include "gguf/model_layout.h"

#include <QString>

class ModelSource;

// strata_planner.read_layout: reads every shard's header and folds tensor sizes and metadata
// into the per-layer byte counts the planner works with.
class LayoutReader {
public:
    static Result<ModelLayout> read(const ModelSource &source, const QString &path);

    // The fold itself, given the first shard's metadata and all shards' tensor sizes.
    static ModelLayout fromShards(const QVariantMap &metadata, const QMap<QString, qint64> &tensorSizes);
};

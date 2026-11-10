#pragma once

#include "common/result.h"

#include <QString>
#include <QStringList>

class ModelSource;

// strata_planner.shard_paths: a split model "<base>-00001-of-00004.gguf" expands to all of its
// shards; any other path is its own single shard.
class ShardFinder {
public:
    static Result<QStringList> shardPaths(const ModelSource &source, const QString &path);
};

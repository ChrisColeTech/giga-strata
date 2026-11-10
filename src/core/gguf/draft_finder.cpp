#include "gguf/draft_finder.h"

#include "common/constants.h"
#include "common/path_key.h"
#include "gguf/gguf_reader.h"
#include "gguf/model_source.h"
#include "gguf/shard_finder.h"

#include <QSet>

namespace {

struct TargetModel {
    QString arch;
    QVariant blockCount;
    QSet<QString> ownKeys;
};

std::optional<TargetModel> readTarget(const ModelSource &source, const QString &modelPath) {
    const auto shards = ShardFinder::shardPaths(source, modelPath);
    if (!shards.ok()) {
        return std::nullopt;
    }
    const auto first = GgufReader::read(source, shards.value().constFirst());
    if (!first.ok()) {
        return std::nullopt;
    }
    TargetModel target;
    target.arch = first.value().metadata.value(QStringLiteral("general.architecture")).toString();
    target.blockCount = first.value().metadata.value(target.arch + QStringLiteral(".block_count"));
    for (const QString &shard : shards.value()) {
        target.ownKeys.insert(path_key::of(shard));
    }
    return target;
}

bool isDraftFor(const QVariantMap &candidate, const TargetModel &target) {
    const qint64 nextn = candidate.value(target.arch + QStringLiteral(".nextn_predict_layers")).toLongLong();
    if (candidate.value(QStringLiteral("general.architecture")).toString() != target.arch || nextn <= 0) {
        return false;
    }
    const QVariant blockCount = candidate.value(target.arch + QStringLiteral(".block_count"));
    return target.blockCount.isValid() && blockCount.isValid() &&
           blockCount.toLongLong() == target.blockCount.toLongLong() + nextn;
}

} // namespace

std::optional<QString> DraftFinder::find(const ModelSource &source, const QString &modelPath,
                                         const QStringList &searchDirectories) {
    const auto target = readTarget(source, modelPath);
    if (!target) {
        return std::nullopt;
    }
    QStringList directories{path_key::parentDirectory(modelPath)};
    for (const QString &directory : searchDirectories) {
        if (source.isDirectory(directory)) {
            directories.append(directory);
        }
    }
    QSet<QString> seen;
    for (const QString &directory : directories) {
        for (const QString &candidate : source.listFiles(directory, QStringLiteral("*.gguf"), true)) {
            const QString key = path_key::of(candidate);
            const auto size = source.fileSize(candidate);
            if (target->ownKeys.contains(key) || seen.contains(key) || !size || *size > constants::MAX_DRAFT_FILE_BYTES) {
                continue;
            }
            seen.insert(key);
            const auto shard = GgufReader::read(source, candidate);
            if (shard.ok() && isDraftFor(shard.value().metadata, *target)) {
                return candidate;
            }
        }
    }
    return std::nullopt;
}

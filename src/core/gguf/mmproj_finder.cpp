#include "gguf/mmproj_finder.h"

#include "common/constants.h"
#include "common/path_key.h"
#include "gguf/gguf_reader.h"
#include "gguf/model_source.h"
#include "gguf/shard_finder.h"

#include <QSet>

namespace {

// Python `a == b` on two metadata values where both may be missing (None == None is True).
bool pythonEquals(const QVariant &left, const QVariant &right) {
    const bool isLeftNone = !left.isValid() || left.isNull();
    const bool isRightNone = !right.isValid() || right.isNull();
    if (isLeftNone || isRightNone) {
        return isLeftNone == isRightNone;
    }
    return left.toLongLong() == right.toLongLong();
}

} // namespace

bool MmprojFinder::matches(const ModelSource &source, const QString &modelPath, const QString &mmprojPath) {
    const auto projector = GgufReader::read(source, mmprojPath);
    const auto shards = ShardFinder::shardPaths(source, modelPath);
    if (!projector.ok() || !shards.ok()) {
        return false;
    }
    const auto model = GgufReader::read(source, shards.value().constFirst());
    if (!model.ok()) {
        return false;
    }
    const QVariantMap &projectorMeta = projector.value().metadata;
    if (projectorMeta.value(QStringLiteral("general.architecture")).toString() != QLatin1String("clip")) {
        return false;
    }
    const QString arch = model.value().metadata.value(QStringLiteral("general.architecture")).toString();
    return pythonEquals(projectorMeta.value(QStringLiteral("clip.vision.projection_dim")),
                        model.value().metadata.value(arch + QStringLiteral(".embedding_length")));
}

std::optional<QString> MmprojFinder::find(const ModelSource &source, const QString &modelPath,
                                          const QStringList &searchDirectories) {
    const QString here = path_key::parentDirectory(modelPath);
    QStringList directories{here, path_key::join(path_key::parentDirectory(here), QStringLiteral("models"))};
    directories += searchDirectories;
    QSet<QString> seen;
    for (const QString &directory : directories) {
        if (!source.isDirectory(directory)) {
            continue;
        }
        for (const QString &candidate : source.listFiles(directory, QStringLiteral("*mmproj*.gguf"), false)) {
            const QString key = path_key::of(candidate);
            if (seen.contains(key)) {
                continue;
            }
            seen.insert(key);
            if (matches(source, modelPath, candidate)) {
                return candidate;
            }
        }
    }
    return std::nullopt;
}

std::optional<qint64> MmprojFinder::reserveBytes(const ModelSource &source, const QString &mmprojPath) {
    const auto size = source.fileSize(mmprojPath);
    if (!size) {
        return std::nullopt;
    }
    return *size + 1024 * constants::MIB;
}

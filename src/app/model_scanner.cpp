#include "model_scanner.h"

#include "common/model_names.h"
#include "common/path_key.h"
#include "gguf/draft_finder.h"
#include "gguf/gguf_reader.h"
#include "gguf/layout_reader.h"
#include "gguf/mmproj_finder.h"
#include "gguf/model_source.h"
#include "gguf/quant_name.h"
#include "gguf/shard_finder.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>

namespace {

// An MTP-only file cannot be served on its own (llama-server: "unknown model architecture:
// 'qwen4exp-mtp'"), even when it is no library model's draft: an "-mtp" architecture, or a
// single file with next-token-prediction layers and no trunk layer 0. Split sets are exempt
// from the trunk test: their first shard may hold only metadata (GLM-5.3 UD-IQ3_XXS does).
bool isMtpOnly(const GgufShard &shard, const QString &arch, const QString &path) {
    if (arch.endsWith(QLatin1String("-mtp"))) {
        return true;
    }
    static const QRegularExpression splitPattern(QStringLiteral("-\\d{5}-of-\\d{5}\\.gguf$"),
                                                 QRegularExpression::CaseInsensitiveOption);
    if (splitPattern.match(path).hasMatch()) {
        return false;
    }
    const qint64 nextn = shard.metadata.value(arch + QStringLiteral(".nextn_predict_layers")).toLongLong();
    const bool hasTrunk = std::any_of(shard.tensorSizes.keyBegin(), shard.tensorSizes.keyEnd(),
                                      [](const QString &name) { return name.startsWith(QLatin1String("blk.0.")); });
    return nextn > 0 && !hasTrunk;
}

// Files that open as GGUF and are neither projectors nor MTP-only, deduplicated across
// overlapping folders.
QStringList candidates(const ModelSource &source, const QStringList &folders) {
    QStringList found;
    QSet<QString> seen;
    for (const QString &folder : folders) {
        for (const QString &path : source.listFiles(folder, QStringLiteral("*.gguf"), true)) {
            const QString key = path_key::of(path);
            if (seen.contains(key) || ModelScanner::isLaterShard(path)) {
                continue;
            }
            seen.insert(key);
            const auto shard = GgufReader::read(source, path);
            const QString arch = shard.ok() ? shard.value().metadata.value(QStringLiteral("general.architecture")).toString()
                                            : QString();
            if (shard.ok() && arch != QLatin1String("clip") && !isMtpOnly(shard.value(), arch, path)) {
                found.append(path);
            }
        }
    }
    return found;
}

std::optional<ModelEntry> entryFor(const ModelSource &source, const QString &path, const QString &usageDirectory,
                                   bool hasDraft) {
    const auto layout = LayoutReader::read(source, path);
    const auto shards = ShardFinder::shardPaths(source, path);
    const auto header = GgufReader::read(source, path);
    if (!layout.ok() || !shards.ok() || !header.ok()) {
        return std::nullopt;
    }
    ModelEntry entry;
    entry.name = ModelNames::modelStem(path);
    entry.path = path;
    entry.type = layout.value().arch;
    entry.quant = QuantName::of(header.value().metadata.value(QStringLiteral("general.file_type")),
                                ModelNames::baseName(path));
    for (const QString &shard : shards.value()) {
        entry.sizeBytes += source.fileSize(shard).value_or(0);
    }
    entry.experts = layout.value().expertCount;
    entry.files = int(shards.value().size());
    if (const auto modified = source.modifiedTimeNs(path)) {
        entry.modified = QDateTime::fromMSecsSinceEpoch(*modified / 1000000);
    }
    entry.hasDraft = hasDraft;
    entry.hasProjector = MmprojFinder::find(source, path, {}).has_value();
    entry.hasUsage = QFileInfo::exists(ModelNames::usagePath(usageDirectory, path));
    return entry;
}

} // namespace

QVector<ModelEntry> ModelScanner::scan(const ModelSource &source, const QStringList &folders,
                                       const QString &usageDirectory) {
    const QStringList files = candidates(source, folders);
    // A file some other model would speculate with is that model's draft, not a model of its own.
    QHash<QString, bool> hasDraft;
    QSet<QString> drafts;
    for (const QString &path : files) {
        const auto draft = DraftFinder::find(source, path, {});
        hasDraft.insert(path, draft.has_value());
        if (draft) {
            drafts.insert(path_key::of(*draft));
        }
    }
    QVector<ModelEntry> entries;
    for (const QString &path : files) {
        if (drafts.contains(path_key::of(path))) {
            continue;
        }
        if (const auto entry = entryFor(source, path, usageDirectory, hasDraft.value(path))) {
            entries.append(*entry);
        }
    }
    return entries;
}

bool ModelScanner::isLaterShard(const QString &path) {
    static const QRegularExpression SHARD(QStringLiteral("-(\\d{5})-of-\\d{5}\\.gguf$"),
                                          QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = SHARD.match(path);
    return match.hasMatch() && match.captured(1).toInt() > 1;
}

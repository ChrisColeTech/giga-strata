#include "gguf/layout_reader.h"

#include "gguf/gguf_reader.h"
#include "gguf/shard_finder.h"

#include <QRegularExpression>
#include <QVariantList>

#include <algorithm>

namespace {

// Python `meta.get(f"{arch}.{key}", fallback)`.
class ArchMetadata {
public:
    ArchMetadata(const QVariantMap &metadata, const QString &arch) : m_metadata(metadata), m_prefix(arch + u'.') {}

    QVariant get(const QString &key, const QVariant &fallback = QVariant()) const {
        return m_metadata.value(m_prefix + key, fallback);
    }
    // Python `g(key, 0) or 0`: missing, null, 0 and empty all become 0.
    qint64 integerOrZero(const QString &key) const { return get(key).toLongLong(); }

private:
    const QVariantMap &m_metadata;
    QString m_prefix;
};

qint64 maxOfScalarOrList(const QVariant &value) {
    if (value.typeId() != QMetaType::QVariantList) {
        return value.toLongLong();
    }
    qint64 best = 0;
    const QVariantList items = value.toList();
    for (int i = 0; i < items.size(); ++i) {
        best = i == 0 ? items[i].toLongLong() : std::max(best, items[i].toLongLong());
    }
    return best;
}

int layerCountFromTensors(const QMap<QString, qint64> &tensorSizes) {
    static const QRegularExpression layerPattern(QStringLiteral("^blk\\.(\\d+)\\."));
    int highest = -1;
    for (auto it = tensorSizes.cbegin(); it != tensorSizes.cend(); ++it) {
        const auto match = layerPattern.match(it.key());
        if (match.hasMatch()) {
            highest = std::max(highest, match.captured(1).toInt());
        }
    }
    return highest + 1;
}

// GLM turns end on <|user|> (and <|observation|> for tools); some GGUFs mark only
// <|endoftext|> as EOS, so the model writes the next user turn itself.
QStringList stopTokenOverrides(const QVariantMap &metadata) {
    const QVariantList tokens = metadata.value(QStringLiteral("tokenizer.ggml.tokens")).toList();
    if (metadata.value(QStringLiteral("tokenizer.ggml.pre")).toString() != QLatin1String("glm4") || tokens.isEmpty()) {
        return {};
    }
    qint64 userId = -1;
    qint64 observationId = -1;
    for (int i = 0; i < tokens.size(); ++i) {
        const QString token = tokens[i].toString();
        userId = token == QLatin1String("<|user|>") ? i : userId;
        observationId = token == QLatin1String("<|observation|>") ? i : observationId;
    }
    const QVariant eos = metadata.value(QStringLiteral("tokenizer.ggml.eos_token_id"));
    if (userId < 0 || (eos.isValid() && !eos.isNull() && eos.toLongLong() == userId)) {
        return {};
    }
    QStringList fixes{QStringLiteral("tokenizer.ggml.eos_token_id=int:%1").arg(userId)};
    if (observationId >= 0 && !metadata.contains(QStringLiteral("tokenizer.ggml.eot_token_id"))) {
        fixes.append(QStringLiteral("tokenizer.ggml.eot_token_id=int:%1").arg(observationId));
    }
    return fixes;
}

QVector<qint64> kvBytesPerToken(const ArchMetadata &meta, int layerCount) {
    const qint64 headCount = maxOfScalarOrList(meta.get(QStringLiteral("attention.head_count"), 1));
    const qint64 embedding = meta.get(QStringLiteral("embedding_length"), 0).toLongLong();
    const QVariant kvHeads = meta.get(QStringLiteral("attention.head_count_kv"), headCount);
    const qint64 keyLength = meta.get(QStringLiteral("attention.key_length"), embedding / std::max<qint64>(headCount, 1)).toLongLong();
    const qint64 valueLength = meta.get(QStringLiteral("attention.value_length"), keyLength).toLongLong();
    const qint64 loraRank = meta.integerOrZero(QStringLiteral("attention.kv_lora_rank"));
    const qint64 interval = meta.integerOrZero(QStringLiteral("full_attention_interval"));
    const qint64 ropeDimensions = meta.get(QStringLiteral("rope.dimension_count"), 64).toLongLong();
    const bool isPerLayer = kvHeads.typeId() == QMetaType::QVariantList;
    const QVariantList kvHeadList = kvHeads.toList();

    QVector<qint64> kv;
    for (int layer = 0; layer < layerCount; ++layer) {
        const qint64 heads = isPerLayer ? kvHeadList.value(layer).toLongLong() : kvHeads.toLongLong();
        if (heads == 0 || (interval != 0 && (layer + 1) % interval != 0)) {
            kv.append(0); // recurrent / linear-attention layer: no growing KV cache
        } else if (loraRank != 0) {
            kv.append((loraRank + ropeDimensions) * 2); // MLA latent cache
        } else {
            kv.append(heads * (keyLength + valueLength) * 2);
        }
    }
    return kv;
}

} // namespace

ModelLayout LayoutReader::fromShards(const QVariantMap &metadata, const QMap<QString, qint64> &tensorSizes) {
    static const QRegularExpression layerPattern(QStringLiteral("^blk\\.(\\d+)\\."));
    static const QRegularExpression expertPattern(QStringLiteral("^blk\\.(\\d+)\\.ffn_.*exps"));

    ModelLayout layout;
    layout.arch = metadata.value(QStringLiteral("general.architecture"), QStringLiteral("?")).toString();
    const ArchMetadata meta(metadata, layout.arch);
    const qint64 blockCount = meta.integerOrZero(QStringLiteral("block_count"));
    layout.skippedLayerCount = int(meta.integerOrZero(QStringLiteral("nextn_predict_layers")));
    layout.layerCount = int(blockCount != 0 ? blockCount : layerCountFromTensors(tensorSizes)) - layout.skippedLayerCount;
    layout.denseBytes.fill(0, layout.layerCount);
    layout.expertBytes.fill(0, layout.layerCount);

    for (auto it = tensorSizes.cbegin(); it != tensorSizes.cend(); ++it) {
        layout.totalBytes += it.value();
        const auto match = layerPattern.match(it.key());
        if (match.hasMatch()) {
            const int layer = match.captured(1).toInt();
            if (layer < layout.layerCount) {
                (expertPattern.match(it.key()).hasMatch() ? layout.expertBytes : layout.denseBytes)[layer] += it.value();
            }
        } else if (it.key().startsWith(QLatin1String("output"))) {
            layout.outputBytes += it.value();
        } else {
            layout.cpuGlobalBytes += it.value();
        }
    }
    if (!tensorSizes.contains(QStringLiteral("output.weight"))) {
        // tied embeddings: llama.cpp puts a copy of token_embd on the GPU as the output head
        layout.outputBytes += tensorSizes.value(QStringLiteral("token_embd.weight"), 0);
    }

    layout.kvBytesPerToken = kvBytesPerToken(meta, layout.layerCount);
    layout.expertCount = int(meta.integerOrZero(QStringLiteral("expert_count")));
    layout.expertUsedCount = int(meta.integerOrZero(QStringLiteral("expert_used_count")));
    layout.embeddingLength = meta.get(QStringLiteral("embedding_length"), 0).toLongLong();
    const qint64 tokenCount = metadata.value(QStringLiteral("tokenizer.ggml.tokens")).toList().size();
    layout.vocabSize = tokenCount != 0 ? tokenCount : meta.get(QStringLiteral("vocab_size"), 0).toLongLong();
    layout.stopTokenOverrides = stopTokenOverrides(metadata);
    layout.hasIndexer = meta.get(QStringLiteral("attention.indexer.head_count")).isValid();
    return layout;
}

Result<ModelLayout> LayoutReader::read(const ModelSource &source, const QString &path) {
    const auto shards = ShardFinder::shardPaths(source, path);
    if (!shards.ok()) {
        return Result<ModelLayout>::failure(shards.error());
    }
    QVariantMap metadata;
    QMap<QString, qint64> tensorSizes;
    for (int i = 0; i < shards.value().size(); ++i) {
        auto shard = GgufReader::read(source, shards.value()[i]);
        if (!shard.ok()) {
            return Result<ModelLayout>::failure(shard.error());
        }
        if (i == 0) {
            metadata = shard.value().metadata;
        }
        tensorSizes.insert(shard.value().tensorSizes);
    }
    return Result<ModelLayout>::success(fromShards(metadata, tensorSizes));
}

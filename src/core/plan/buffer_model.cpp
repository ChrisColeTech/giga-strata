#include "plan/buffer_model.h"

#include "common/constants.h"

#include <algorithm>

namespace {

constexpr qint64 BYTES_PER_LOGIT = 6;
constexpr qint64 ACTIVATION_FACTOR = 4 * 48;
constexpr qint64 DRAFT_KV_BYTES_PER_TOKEN = 1800;
constexpr qint64 DRAFT_FIXED_BYTES = 512 * constants::MIB;

qint64 orDefault(qint64 value, qint64 fallback) {
    return value != 0 ? value : fallback;
}

qint64 indexerScoreBytes(const ModelLayout &layout, qint64 microBatch, qint64 contextSize) {
    return layout.hasIndexer ? microBatch * contextSize * BYTES_PER_LOGIT : 0;
}

} // namespace

qint64 BufferModel::computeBufferBytes(const ModelLayout &layout, qint64 microBatch, bool isOutputGpu,
                                       qint64 contextSize) {
    const qint64 activations =
        microBatch * orDefault(layout.embeddingLength, constants::DEFAULT_EMBEDDING_LENGTH) * ACTIVATION_FACTOR;
    const qint64 logits =
        isOutputGpu ? microBatch * orDefault(layout.vocabSize, constants::DEFAULT_VOCAB_SIZE) * BYTES_PER_LOGIT : 0;
    return activations + std::max(logits, indexerScoreBytes(layout, microBatch, contextSize));
}

qint64 BufferModel::draftReserveBytes(const ModelLayout &layout, qint64 draftFileBytes, qint64 microBatch,
                                      qint64 contextSize) {
    const qint64 logits = microBatch * orDefault(layout.vocabSize, constants::DEFAULT_VOCAB_SIZE) * BYTES_PER_LOGIT;
    return draftFileBytes + std::max(logits, indexerScoreBytes(layout, microBatch, contextSize)) +
           contextSize * DRAFT_KV_BYTES_PER_TOKEN + DRAFT_FIXED_BYTES;
}

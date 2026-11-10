#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// strata_planner.ModelLayout: what the planner needs from a GGUF, read without loading weights.
// Field order and meaning mirror the Python dataclass so golden JSON maps one to one
// (Python name in brackets where it differs).
struct ModelLayout {
    QString arch;
    int layerCount = 0;               // [n_layer] layers that actually run
    int skippedLayerCount = 0;        // [n_skipped] trailing MTP layers llama.cpp loads but skips
    int expertCount = 0;              // [n_expert]
    int expertUsedCount = 0;          // [n_expert_used]
    qint64 totalBytes = 0;            // [total_bytes] all tensors, all shards
    QVector<qint64> denseBytes;       // [dense] per layer, non-expert tensors
    QVector<qint64> expertBytes;      // [experts] per layer, expert tensors
    qint64 outputBytes = 0;           // [output_bytes] output head, lives on the last GPU
    qint64 cpuGlobalBytes = 0;        // [cpu_global_bytes] token embeddings etc., kept on the CPU
    QVector<qint64> kvBytesPerToken;  // [kv_per_token] per layer, f16 K+V
    qint64 embeddingLength = 0;       // [n_embd]
    qint64 vocabSize = 0;             // [n_vocab]
    QStringList stopTokenOverrides;   // [eog_overrides] --override-kv values for wrong stop tokens
    bool hasIndexer = false;          // [has_indexer] sparse-attention indexer present
};

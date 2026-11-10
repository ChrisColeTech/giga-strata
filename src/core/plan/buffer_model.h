#pragma once

#include "gguf/model_layout.h"

// strata_planner's VRAM estimates for llama.cpp buffers, fitted to measured GLM-5.3 runs:
// activations scale with micro-batch x embedding; the output GPU adds logits; a sparse-attention
// indexer adds a micro-batch x context score matrix sharing the logits' allocator space.
class BufferModel {
public:
    static qint64 computeBufferBytes(const ModelLayout &layout, qint64 microBatch, bool isOutputGpu, qint64 contextSize);

    // VRAM an MTP draft needs on its GPU: its file, its KV cache and a compute buffer holding
    // the target's logits or indexer scores. `layout` is the target model's.
    static qint64 draftReserveBytes(const ModelLayout &layout, qint64 draftFileBytes, qint64 microBatch,
                                    qint64 contextSize);
};

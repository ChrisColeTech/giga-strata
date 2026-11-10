#pragma once

#include "common/result.h"
#include "gguf/model_layout.h"
#include "hardware/gpu.h"
#include "plan/layer_placement.h"

#include <QVector>

// strata_planner._place.
class LayerPlacer {
public:
    // kvBytes: per-layer KV cache for the chosen context. reserveLastBytes: extra VRAM kept free
    // on the last GPU (MTP draft, vision projector).
    static Result<LayerPlacement> place(const ModelLayout &layout, const QVector<Gpu> &gpus,
                                        const QVector<qint64> &kvBytes, qint64 marginMib, qint64 microBatch,
                                        qint64 reserveLastBytes, qint64 contextSize);
};

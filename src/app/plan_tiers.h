#pragma once

#include "gguf/model_layout.h"
#include "hardware/hardware_snapshot.h"
#include "plan/plan_model.h"

#include <QString>
#include <QVariantList>
#include <QVector>

// What a plan puts where, in the shapes the memory bars and the summary line bind to
// (05 section 3): one tier per selected GPU in CUDA order, then RAM, then SSD when the RAM tier
// is streamed. Every number comes from the plan and the snapshot it was made with.
class PlanTiers {
public:
    struct Input {
        const ModelLayout *layout = nullptr;
        const Plan *plan = nullptr;
        QVector<Gpu> gpus;            // the selected GPUs, CUDA order
        qint64 contextSize = 0;
        qint64 reserveLastBytes = 0;  // draft + projector on the last GPU
        std::optional<qint64> freeRamBytes;
    };

    // {label, tierIndex, usedBytes, capacityBytes, layerText}; capacity 0 when unknown.
    static QVariantList tiers(const Input &input);

    // "45 layers: 22 on GPU0, 21 on GPU1, 2 in RAM" (", 3 split" when layers are split by the hot file).
    static QString summary(const Input &input);
};

#pragma once

#include <QStringList>
#include <QVector>

// strata_planner._place result: a contiguous layer split across GPUs, each GPU's leftover VRAM
// filled with whole expert layers from its own range.
struct LayerPlacement {
    QVector<int> layerCounts;             // [counts] per GPU, CUDA order
    QVector<int> rangeStarts;             // first layer of each GPU's range
    QVector<QVector<int>> gpuExpertLayers; // [gpu_experts] in the order chosen (last layer first)
    QVector<int> cpuExpertLayers;         // [cpu_layers] sorted
    QVector<qint64> spareBytes;           // [avail] per GPU after dense + KV (+ output), never negative
    qint64 gpuExpertBytes = 0;            // [gpu_exp_bytes]
    QStringList lines;
    QStringList warnings;
};

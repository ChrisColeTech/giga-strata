#pragma once

#include "moe/hot_map.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

// strata_planner.Plan plus the placement structure behind it, which the strata view draws.
// args/report/warnings/useMmap/microBatch/arch are golden-compared against Python verbatim.
struct Plan {
    QStringList args;
    bool useMmap = false;
    int microBatch = 2048;       // [ubatch]
    QString arch;
    QStringList report;
    QStringList warnings;

    // Placement detail (not in the Python Plan; derived from the same _place result).
    QVector<int> gpuLayerCounts;           // [counts] contiguous layers per selected GPU, in CUDA order
    QVector<QVector<int>> gpuExpertLayers; // [gpu_experts] whole-layer experts on each GPU
    QVector<int> cpuExpertLayers;          // [cpu_layers] sorted, whole expert layers left in RAM
    qint64 ramTierBytes = 0;               // [cpu_bytes] bytes the RAM tier must hold

    // Per-expert placement (P5b); all empty when whole-layer. hotExperts lists every layer the
    // hot file names, with its VRAM experts; the rest of those layers stays in RAM.
    HotMap hotExperts;
    QString hotFilePath;                   // [moe_hot_path] passed as --moe-hot
    QByteArray hotFileText;                // write_hot output, or the reused file's text
    bool isHotFileReused = false;          // the cached file still fits: Run must not rewrite it
};

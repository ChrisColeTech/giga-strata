#pragma once

#include "common/result.h"
#include "hardware/gpu.h"

#include <QString>
#include <QStringList>
#include <QVector>

// nvidia-smi's GPU query, exactly as strata_planner.query_gpus asks for and parses it.
class GpuInfo {
public:
    static QStringList nvidiaSmiArguments();
    // One "index, name, free MiB, total MiB" row per line; any malformed row fails the whole
    // read, like the Python int() conversion would.
    static Result<QVector<Gpu>> parseCsv(const QString &csv);
    // "NVIDIA GeForce RTX 5090" -> "RTX 5090": chips and gauges have room for the model only.
    static QString shortName(const QString &name);
};

#include "plan/layer_placer.h"

#include "common/constants.h"
#include "common/python_format.h"
#include "plan/buffer_model.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace {

QVector<qint64> budgets(const ModelLayout &layout, const QVector<Gpu> &gpus, qint64 marginMib, qint64 microBatch,
                        qint64 reserveLastBytes, qint64 contextSize) {
    const int last = int(gpus.size()) - 1;
    QVector<qint64> result;
    for (int gpu = 0; gpu < gpus.size(); ++gpu) {
        const bool isLast = gpu == last;
        const qint64 budget = gpus[gpu].freeMib * constants::MIB - marginMib * constants::MIB -
                              BufferModel::computeBufferBytes(layout, microBatch, isLast, contextSize) -
                              (isLast ? reserveLastBytes : 0);
        result.append(std::max<qint64>(budget, 0));
    }
    return result;
}

// Contiguous split proportional to budget. Python round() is half-to-even, which nearbyint
// gives under the default rounding mode; std::round would move layer boundaries (02 H1).
QVector<int> layerCounts(int layerCount, const QVector<qint64> &budget) {
    const qint64 total = std::accumulate(budget.cbegin(), budget.cend(), qint64(0));
    QVector<int> counts;
    int assigned = 0;
    for (int gpu = 0; gpu < budget.size(); ++gpu) {
        const bool isLast = gpu == budget.size() - 1;
        const int count = isLast ? layerCount - assigned
                                 : int(std::nearbyint(double(qint64(layerCount) * budget[gpu]) / double(total)));
        counts.append(count);
        assigned += count;
    }
    return counts;
}

qint64 baseBytes(const ModelLayout &layout, const QVector<qint64> &kvBytes, int start, int count, bool isLast) {
    qint64 base = isLast ? layout.outputBytes : 0;
    for (int layer = start; layer < start + count; ++layer) {
        base += layout.denseBytes[layer] + kvBytes[layer];
    }
    return base;
}

} // namespace

Result<LayerPlacement> LayerPlacer::place(const ModelLayout &layout, const QVector<Gpu> &gpus,
                                          const QVector<qint64> &kvBytes, qint64 marginMib, qint64 microBatch,
                                          qint64 reserveLastBytes, qint64 contextSize) {
    using python_format::gib;
    const QVector<qint64> budget = budgets(layout, gpus, marginMib, microBatch, reserveLastBytes, contextSize);
    if (std::accumulate(budget.cbegin(), budget.cend(), qint64(0)) == 0) {
        return Result<LayerPlacement>::failure(QStringLiteral("no free VRAM left after the safety margin and compute buffers"));
    }
    LayerPlacement placement;
    placement.layerCounts = layerCounts(layout.layerCount, budget);
    QVector<bool> isOnGpu(layout.layerCount, false);
    int start = 0;
    for (int gpu = 0; gpu < gpus.size(); ++gpu) {
        const bool isLast = gpu == gpus.size() - 1;
        const int count = placement.layerCounts[gpu];
        const qint64 base = baseBytes(layout, kvBytes, start, count, isLast);
        const qint64 spare = budget[gpu] - base;
        if (spare < 0) {
            placement.warnings.append(QStringLiteral("CUDA%1 (%2): attention/KV alone need %3 but only %4 is free. "
                                                     "Lower the context size, micro-batch or margin.")
                                          .arg(gpu).arg(gpus[gpu].name, gib(base), gib(budget[gpu])));
        }
        QVector<int> chosen;
        qint64 used = 0;
        // fill from the GPU's last layer backwards; every expert layer costs about the same
        for (int layer = start + count - 1; layer >= start; --layer) {
            if (layout.expertBytes[layer] != 0 && used + layout.expertBytes[layer] <= spare) {
                chosen.append(layer);
                used += layout.expertBytes[layer];
                isOnGpu[layer] = true;
            }
        }
        placement.lines.append(QStringLiteral("CUDA%1 %2: layers %3-%4, base %5 + experts of %6 layer(s) %7 / %8 usable "
                                              "(compute buffer ~%9 reserved)")
                                   .arg(gpu).arg(gpus[gpu].name).arg(start).arg(start + count - 1)
                                   .arg(gib(base)).arg(chosen.size()).arg(gib(used), gib(budget[gpu]),
                                        gib(BufferModel::computeBufferBytes(layout, microBatch, isLast, contextSize))));
        placement.gpuExpertLayers.append(chosen);
        placement.rangeStarts.append(start);
        placement.spareBytes.append(std::max<qint64>(spare, 0));
        start += count;
    }
    qint64 cpuExpertBytes = 0;
    for (int layer = 0; layer < layout.layerCount; ++layer) {
        if (layout.expertBytes[layer] != 0 && !isOnGpu[layer]) {
            placement.cpuExpertLayers.append(layer);
            cpuExpertBytes += layout.expertBytes[layer];
        }
    }
    const qint64 totalExpertBytes = std::accumulate(layout.expertBytes.cbegin(), layout.expertBytes.cend(), qint64(0));
    placement.gpuExpertBytes = totalExpertBytes - cpuExpertBytes;
    return Result<LayerPlacement>::success(placement);
}

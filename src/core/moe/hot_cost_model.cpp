#include "moe/hot_cost_model.h"

#include "gguf/model_layout.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

constexpr double BYTES_PER_GIB = 1024.0 * 1024.0 * 1024.0;

QVector<int> iterationOrder(const HotMap &hot, const QVector<int> &layerOrder) {
    return layerOrder.isEmpty() ? QVector<int>(hot.keyBegin(), hot.keyEnd()) : layerOrder;
}

double perExpertBytes(const ModelLayout &layout, int layer) {
    return double(layout.expertBytes.value(layer)) / double(layout.expertCount);
}

} // namespace

double HotCostModel::layerCostMs(const ModelLayout &layout, const QVector<double> &layerCounts, int layer,
                                 int hotCount, double bandwidthGibPerSecond, double layerOverheadMs) {
    if (hotCount >= layout.expertCount) {
        return 0.0;
    }
    const QVector<double> sorted = sortedDescending(layerCounts);
    double total = pythonSum(sorted);
    if (total == 0.0) {
        total = 1.0;
    }
    const double hotSum = pythonSum(sorted.mid(0, hotCount));
    const double coldShare = 1.0 - hotSum / total;
    const double perExpert = perExpertBytes(layout, layer);
    return layerOverheadMs
           + 1000.0 * layout.expertUsedCount * perExpert * coldShare / (bandwidthGibPerSecond * BYTES_PER_GIB);
}

double HotCostModel::predictMs(const ModelLayout &layout, const UsageCounts &counts, const HotMap &hot,
                               const QVector<int> &cpuLayers, double bandwidthGibPerSecond, double layerOverheadMs,
                               const QVector<int> &layerOrder) {
    QVector<double> hotCosts;
    for (const int layer : iterationOrder(hot, layerOrder)) {
        hotCosts.append(layerCostMs(layout, counts.value(layer), layer, int(hot.value(layer).size()),
                                    bandwidthGibPerSecond, layerOverheadMs));
    }
    const QVector<double> uniform(layout.expertCount, 1.0);
    QVector<double> cpuCosts;
    for (const int layer : cpuLayers) {
        cpuCosts.append(layerCostMs(layout, uniform, layer, 0, bandwidthGibPerSecond, layerOverheadMs));
    }
    return pythonSum(hotCosts) + pythonSum(cpuCosts);
}

double HotCostModel::hitRate(const ModelLayout &layout, const UsageCounts &counts, const HotMap &hot,
                             const QVector<int> &layerOrder) {
    double read = 0.0;
    double served = 0.0;
    for (const int layer : iterationOrder(hot, layerOrder)) {
        const QVector<double> layerCounts = counts.value(layer);
        const double perExpert = perExpertBytes(layout, layer);
        read += perExpert;
        QVector<double> hotCounts;
        for (const int expert : hot.value(layer)) {
            hotCounts.append(layerCounts.value(expert));
        }
        double total = pythonSum(layerCounts);
        if (total == 0.0) {
            total = 1.0;
        }
        served += perExpert * pythonSum(hotCounts) / total;
    }
    return read != 0.0 ? served / read : 0.0;
}

double HotCostModel::pythonSum(const QVector<double> &values) {
    if (values.isEmpty()) {
        return 0.0;
    }
    // CPython 3.12 builtin sum: the int start 0 plus the first float is exact, then Neumaier
    // compensation, with the compensation added once at the end when it is finite and non-zero.
    double result = 0.0 + values.first();
    double compensation = 0.0;
    for (qsizetype at = 1; at < values.size(); ++at) {
        const double value = values[at];
        const double next = result + value;
        if (std::fabs(result) >= std::fabs(value)) {
            compensation += (result - next) + value;
        } else {
            compensation += (value - next) + result;
        }
        result = next;
    }
    if (compensation != 0.0 && std::isfinite(compensation)) {
        result += compensation;
    }
    return result;
}

QVector<double> HotCostModel::sortedDescending(const QVector<double> &values) {
    QVector<double> sorted = values;
    std::stable_sort(sorted.begin(), sorted.end(), std::greater<double>());
    return sorted;
}

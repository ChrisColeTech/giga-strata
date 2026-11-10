#pragma once

#include "moe/hot_cost_model.h"

#include <QPair>
#include <QVector>

#include <atomic>

// moe_hot.build_hot (greedy) and build_hot_cost (exact knapsack): which experts of each GPU's
// layers go into that GPU's spare VRAM. GPU ranges are contiguous: rangeStarts[i] and
// layerCounts[i] describe GPU i's layers, spareBytes[i] its budget. The cost functions
// (layerCostMs, predictMs, hitRate) are inherited from HotCostModel so callers can use
// HotBuilder:: for the whole moe_hot API.
class HotBuilder : public HotCostModel {
public:
    // build_hot: per GPU, the experts with the most reads per byte until the budget is full.
    // Every layer with experts and a full set of counts appears, possibly with no hot experts.
    static HotMap buildGreedy(const ModelLayout &layout, const UsageCounts &counts, const QVector<int> &rangeStarts,
                              const QVector<int> &layerCounts, const QVector<qint64> &spareBytes);

    // build_hot_cost: per layer the number of most used experts kept in VRAM that minimizes the
    // summed layer cost within each GPU's budget, VRAM counted in 4 MiB units. allowMixed=false
    // allows only whole layers. Returns the map and the predicted ms; when `cancel` turns true it
    // stops between layers and returns an empty map with +inf (02 D5).
    static QPair<HotMap, double> buildCost(const ModelLayout &layout, const UsageCounts &counts,
                                           const QVector<int> &rangeStarts, const QVector<int> &layerCounts,
                                           const QVector<qint64> &spareBytes, double bandwidthGibPerSecond,
                                           double layerOverheadMs, bool allowMixed,
                                           const std::atomic<bool> *cancel = nullptr);

    // The order Python's dict returned by build_hot_cost iterates in: GPU by GPU, each GPU's
    // layers from last to first. Pass it to hitRate/predictMs for bit-identical sums.
    static QVector<int> costInsertionOrder(const HotMap &hot, const QVector<int> &rangeStarts,
                                           const QVector<int> &layerCounts);

    // -(-k * per_expert // unit): Python float floor division, then negated, as a unit count.
    static qint64 unitsFor(int expertCount, double perExpertBytes);
};

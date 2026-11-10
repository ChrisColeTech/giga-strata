#pragma once

#include "moe/hot_map.h"
#include "moe/usage_counts.h"

#include <QVector>

struct ModelLayout;

// moe_hot.layer_cost_ms / predict_ms / hit_rate: what a hot map is predicted to cost per decoded
// token and how much of the expert traffic it serves from VRAM. Float operations follow the
// Python expressions term by term so results match to the last bit (02 H4).
class HotCostModel {
public:
    // Predicted decode ms/token on the cold experts of one layer whose `hotCount` most used
    // experts are in VRAM; 0 when the whole layer is hot.
    static double layerCostMs(const ModelLayout &layout, const QVector<double> &layerCounts, int layer, int hotCount,
                              double bandwidthGibPerSecond, double layerOverheadMs);

    // predict_ms: the layers of `hot` plus whole layers in RAM (`cpuLayers`, in the given order).
    // `layerOrder` is the iteration order of Python's dict for `hot`; empty means ascending, which
    // is the order of a dict read back from a hot file. Float sums depend on that order.
    static double predictMs(const ModelLayout &layout, const UsageCounts &counts, const HotMap &hot,
                            const QVector<int> &cpuLayers, double bandwidthGibPerSecond, double layerOverheadMs,
                            const QVector<int> &layerOrder = {});

    // hit_rate: share of routed expert bytes per token served from VRAM. `layerOrder` as above:
    // build_hot_cost returns its dict in costInsertionOrder (hot_builder.h), not ascending.
    static double hitRate(const ModelLayout &layout, const UsageCounts &counts, const HotMap &hot,
                          const QVector<int> &layerOrder = {});

    // Python 3.12 sum() over floats: Neumaier-compensated, not a plain running total. Every
    // sum(...) of counts in the reference goes through this.
    static double pythonSum(const QVector<double> &values);

    // sorted(values, reverse=True)
    static QVector<double> sortedDescending(const QVector<double> &values);
};

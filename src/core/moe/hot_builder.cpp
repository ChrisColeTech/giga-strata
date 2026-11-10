#include "moe/hot_builder.h"

// Optimized in every configuration: see giga_knapsack in src/core/CMakeLists.txt. Only speed
// changes; the float operations and their order are the same, so results stay bit-identical.
#pragma optimize("gt", on)

#include "common/constants.h"
#include "gguf/model_layout.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace {

constexpr double BYTES_PER_GIB = 1024.0 * 1024.0 * 1024.0;

struct GreedyItem {
    double score;
    int layer;
    int expert;
    double size;
};

// One knapsack stage: the least cost per budget after this layer, and the k chosen for it.
struct KnapsackStage {
    QVector<double> best;
    QVector<qint16> pick;
};

double perExpertBytes(const ModelLayout &layout, int layer) {
    return double(layout.expertBytes.value(layer)) / double(layout.expertCount);
}

bool hasFullCounts(const ModelLayout &layout, const UsageCounts &counts, int layer) {
    const QVector<double> layerCounts = counts.value(layer);
    return layout.expertBytes.value(layer) != 0 && !layerCounts.isEmpty() && layerCounts.size() == layout.expertCount;
}

// CPython float_floor_div (_float_div_mod): fmod, divide, then snap to the nearest integral
// value. Not the same as floor(a / b) for every input, so it is reproduced as written.
double pythonFloorDivide(double numerator, double denominator) {
    double remainder = std::fmod(numerator, denominator);
    double quotient = (numerator - remainder) / denominator;
    if (remainder != 0.0 && ((denominator < 0) != (remainder < 0))) {
        quotient -= 1.0;
    }
    if (quotient == 0.0) {
        return std::copysign(0.0, numerator / denominator);
    }
    double floored = std::floor(quotient);
    if (quotient - floored > 0.5) {
        floored += 1.0;
    }
    return floored;
}

// The per-layer cost table of build_hot_cost, numpy operation by numpy operation.
QVector<double> layerCostTable(const ModelLayout &layout, const QVector<double> &layerCounts, int layer,
                               double bandwidthGibPerSecond, double layerOverheadMs) {
    const QVector<double> sorted = HotCostModel::sortedDescending(layerCounts);
    double total = HotCostModel::pythonSum(sorted);
    if (total == 0.0) {
        total = 1.0;
    }
    const double readMs =
        1000.0 * layout.expertUsedCount * perExpertBytes(layout, layer) / (bandwidthGibPerSecond * BYTES_PER_GIB);
    QVector<double> cost(sorted.size() + 1);
    double running = 0.0;
    for (qsizetype k = 0; k < cost.size(); ++k) {
        if (k > 0) {
            running = k == 1 ? sorted[0] : running + sorted[k - 1];
        }
        cost[k] = layerOverheadMs + readMs * (1.0 - running / total);
    }
    cost[layout.expertCount] = 0.0;
    return cost;
}

KnapsackStage addLayer(const ModelLayout &layout, const QVector<double> &previous, const QVector<double> &cost,
                       int layer, bool allowMixed) {
    const qsizetype budgetUnits = previous.size();
    KnapsackStage stage{QVector<double>(budgetUnits, std::numeric_limits<double>::infinity()),
                        QVector<qint16>(budgetUnits, 0)};
    const double perExpert = perExpertBytes(layout, layer);
    const int expertCount = layout.expertCount;
    for (int k = 0; k <= expertCount; k = allowMixed || k == expertCount ? k + 1 : expertCount) {
        const qint64 weight = HotBuilder::unitsFor(k, perExpert);
        if (weight >= budgetUnits) {
            break;
        }
        // Raw pointers: this loop runs ~10^8 times per plan, and QVector's checked operator[]
        // makes unoptimized builds minutes slow. Same operations, same order.
        const double *from = previous.constData();
        double *best = stage.best.data();
        qint16 *pick = stage.pick.data();
        const double costOfK = cost[k];
        for (qsizetype units = weight; units < budgetUnits; ++units) {
            const double candidate = from[units - weight] + costOfK;
            if (candidate < best[units]) {
                best[units] = candidate;
                pick[units] = qint16(k);
            }
        }
    }
    return stage;
}

QVector<int> mostUsedFirst(const QVector<double> &layerCounts) {
    QVector<int> order(layerCounts.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return -layerCounts[a] < -layerCounts[b]; });
    return order;
}

} // namespace

HotMap HotBuilder::buildGreedy(const ModelLayout &layout, const UsageCounts &counts, const QVector<int> &rangeStarts,
                               const QVector<int> &layerCounts, const QVector<qint64> &spareBytes) {
    HotMap hot;
    for (qsizetype gpu = 0; gpu < std::min(rangeStarts.size(), spareBytes.size()); ++gpu) {
        QVector<GreedyItem> items;
        for (int layer = rangeStarts[gpu]; layer < rangeStarts[gpu] + layerCounts.value(gpu); ++layer) {
            if (!hasFullCounts(layout, counts, layer)) {
                continue;
            }
            hot.insert(layer, {});
            const QVector<double> layerCountsOf = counts.value(layer);
            double total = pythonSum(layerCountsOf);
            if (total == 0.0) {
                total = 1.0;
            }
            const double perExpert = perExpertBytes(layout, layer);
            for (int expert = 0; expert < layerCountsOf.size(); ++expert) {
                items.append({layerCountsOf[expert] / total / perExpert, layer, expert, perExpert});
            }
        }
        std::stable_sort(items.begin(), items.end(),
                         [](const GreedyItem &a, const GreedyItem &b) { return -a.score < -b.score; });
        double used = 0.0;
        for (const GreedyItem &item : items) {
            if (used + item.size > double(spareBytes[gpu])) {
                continue;
            }
            hot[item.layer].append(item.expert);
            used += item.size;
        }
    }
    for (QVector<int> &experts : hot) {
        std::sort(experts.begin(), experts.end());
    }
    return hot;
}

QPair<HotMap, double> HotBuilder::buildCost(const ModelLayout &layout, const UsageCounts &counts,
                                            const QVector<int> &rangeStarts, const QVector<int> &layerCounts,
                                            const QVector<qint64> &spareBytes, double bandwidthGibPerSecond,
                                            double layerOverheadMs, bool allowMixed, const std::atomic<bool> *cancel) {
    const QPair<HotMap, double> cancelled{HotMap(), std::numeric_limits<double>::infinity()};
    HotMap hot;
    double totalMs = 0.0;
    for (qsizetype gpu = 0; gpu < std::min(rangeStarts.size(), spareBytes.size()); ++gpu) {
        QVector<int> layers;
        for (int layer = rangeStarts[gpu]; layer < rangeStarts[gpu] + layerCounts.value(gpu); ++layer) {
            if (hasFullCounts(layout, counts, layer)) {
                layers.append(layer);
            }
        }
        const qint64 budget = spareBytes[gpu];
        const qint64 unit = constants::KNAPSACK_UNIT_BYTES;
        const qint64 budgetUnits = (budget >= 0 ? budget / unit : -((-budget + unit - 1) / unit)) + 1;
        QVector<double> best(std::max<qint64>(budgetUnits, 0), 0.0);
        QVector<QVector<qint16>> choices;
        for (const int layer : layers) {
            if (cancel && cancel->load()) {
                return cancelled;
            }
            const QVector<double> cost =
                layerCostTable(layout, counts.value(layer), layer, bandwidthGibPerSecond, layerOverheadMs);
            KnapsackStage stage = addLayer(layout, best, cost, layer, allowMixed);
            best = std::move(stage.best);
            choices.append(std::move(stage.pick));
        }
        qsizetype units = best.size() - 1;
        totalMs += layers.isEmpty() ? 0.0 : best[units];
        for (qsizetype at = layers.size() - 1; at >= 0; --at) {
            const int layer = layers[at];
            const int k = choices[at][units];
            units -= qsizetype(unitsFor(k, perExpertBytes(layout, layer)));
            QVector<int> chosen = mostUsedFirst(counts.value(layer)).mid(0, k);
            std::sort(chosen.begin(), chosen.end());
            hot.insert(layer, chosen);
        }
    }
    return {hot, totalMs};
}

QVector<int> HotBuilder::costInsertionOrder(const HotMap &hot, const QVector<int> &rangeStarts,
                                            const QVector<int> &layerCounts) {
    QVector<int> order;
    for (qsizetype gpu = 0; gpu < rangeStarts.size(); ++gpu) {
        for (int layer = rangeStarts[gpu] + layerCounts.value(gpu) - 1; layer >= rangeStarts[gpu]; --layer) {
            if (hot.contains(layer)) {
                order.append(layer);
            }
        }
    }
    return order;
}

qint64 HotBuilder::unitsFor(int expertCount, double perExpertBytes) {
    const double negative = double(-expertCount) * perExpertBytes;
    return -qint64(pythonFloorDivide(negative, double(constants::KNAPSACK_UNIT_BYTES)));
}

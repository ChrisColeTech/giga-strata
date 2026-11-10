#include "plan/expert_placer.h"

#include "common/constants.h"
#include "common/python_format.h"
#include "moe/hot_builder.h"
#include "plan/hot_reuse.h"

#include <QLocale>

#include <algorithm>
#include <limits>

namespace {

using python_format::fixed;
using python_format::gib;

constexpr qint64 SPLIT_RESERVE_SLACK_BYTES = 256 * constants::MIB;
// Without a stable reserve after this many rebuilds the split plan is dropped for whole layers.
constexpr int RESERVE_ATTEMPTS = 4;
constexpr double BANDWIDTH = constants::DEFAULT_BANDWIDTH_GIB_PER_SECOND;
constexpr double OVERHEAD = constants::DEFAULT_LAYER_OVERHEAD_MS;

// A hot map with the iteration order Python's dict had: hit_rate sums floats in that order.
struct OrderedHot {
    HotMap hot;
    QVector<int> order;  // empty = ascending layers
    double milliseconds = 0.0;
};

QString cancelledMessage() {
    return QStringLiteral("Planning was cancelled.");
}

bool isCancelled(const std::atomic<bool> *cancel) {
    return cancel && cancel->load();
}

// Python repr of a float, which the report prints for the two cost constants ("29.7").
QString pythonRepr(double value) {
    return QString::number(value, 'g', QLocale::FloatingPointShortest);
}

OrderedHot costResult(const QPair<HotMap, double> &built, const LayerPlacement &placement) {
    return {built.first, HotBuilder::costInsertionOrder(built.first, placement.rangeStarts, placement.layerCounts),
            built.second};
}

// The cost branch: rebuild with the first GPU's reserve sized to the largest split cold side
// until it is stable, then keep whichever of that and whole layers predicts less.
Result<OrderedHot> buildCostAware(const ModelLayout &layout, const UsageCounts &counts, const LayerPlacement &placement,
                                  const std::atomic<bool> *cancel, QStringList &report) {
    const QVector<int> &starts = placement.rangeStarts;
    const QVector<int> &sizes = placement.layerCounts;
    QVector<qint64> spare = placement.spareBytes;
    qint64 reserve = constants::MOE_HOT_GPU0_EXTRA_BYTES;
    OrderedHot mixed;
    bool isStable = false;
    for (int attempt = 0; attempt < RESERVE_ATTEMPTS && !isStable; ++attempt) {
        spare[0] = std::max<qint64>(placement.spareBytes[0] - reserve, 0);
        mixed = costResult(HotBuilder::buildCost(layout, counts, starts, sizes, spare, BANDWIDTH, OVERHEAD, true, cancel),
                           placement);
        if (isCancelled(cancel)) {
            return Result<OrderedHot>::failure(cancelledMessage());
        }
        const qint64 needed = ExpertPlacer::largestColdSide(layout, mixed.hot) + SPLIT_RESERVE_SLACK_BYTES;
        isStable = needed <= reserve;
        reserve = isStable ? reserve : needed;
    }
    if (!isStable) {
        mixed.milliseconds = std::numeric_limits<double>::infinity();
    }
    const OrderedHot whole = costResult(
        HotBuilder::buildCost(layout, counts, starts, sizes, placement.spareBytes, BANDWIDTH, OVERHEAD, false, cancel),
        placement);
    if (isCancelled(cancel)) {
        return Result<OrderedHot>::failure(cancelledMessage());
    }
    const OrderedHot &chosen = whole.milliseconds <= mixed.milliseconds ? whole : mixed;
    const double baseMs = HotBuilder::predictMs(layout, counts, {}, placement.cpuExpertLayers, BANDWIDTH, OVERHEAD);
    report.append(QStringLiteral("Cost-aware placement: predicted %1 ms/token on cold experts and round trips "
                                 "(whole-layer plan %2), %3 GiB/s RAM, %4 ms per layer with cold experts")
                      .arg(fixed(chosen.milliseconds, 1), fixed(baseMs, 1), pythonRepr(BANDWIDTH), pythonRepr(OVERHEAD)));
    return Result<OrderedHot>::success(chosen);
}

QString reusedReport(const ModelLayout &layout, const UsageCounts &counts, const HotMap &hot) {
    HotMap counted;
    for (auto layer = hot.cbegin(); layer != hot.cend(); ++layer) {
        if (counts.contains(layer.key())) {
            counted.insert(layer.key(), layer.value());
        }
    }
    const double ms = HotBuilder::predictMs(layout, counts, counted, {}, BANDWIDTH, OVERHEAD);
    return QStringLiteral("Cost-aware placement: reusing the cached hot file (it still fits), predicted %1 ms/token on "
                          "cold experts and round trips")
        .arg(fixed(ms, 1));
}

Result<OrderedHot> chooseHot(const ModelLayout &layout, const UsageCounts &counts, const LayerPlacement &placement,
                             const PerExpertRequest &request, const std::atomic<bool> *cancel, ExpertPlacement &out) {
    if (request.cachedHotFile) {
        const auto reused = HotReuse::reuse(layout, *request.cachedHotFile, placement, request.isCostAware);
        if (reused) {
            out.isReused = true;
            out.reportLines.append(reusedReport(layout, counts, *reused));
            return Result<OrderedHot>::success({*reused, {}, 0.0});
        }
    }
    if (request.isCostAware) {
        return buildCostAware(layout, counts, placement, cancel, out.reportLines);
    }
    // prompt batches offload every cold expert matmul to the first GPU, growing its compute buffer
    QVector<qint64> spare = placement.spareBytes;
    spare[0] = std::max<qint64>(spare[0] - constants::MOE_HOT_GPU0_EXTRA_BYTES, 0);
    return Result<OrderedHot>::success(
        {HotBuilder::buildGreedy(layout, counts, placement.rangeStarts, placement.layerCounts, spare), {}, 0.0});
}

// Layers the hot file names leave the -ot / RAM lists; their placement is the file's.
void applyHot(const ModelLayout &layout, const LayerPlacement &placement, ExpertPlacement &out) {
    qint64 hotTotal = 0;
    for (auto layer = out.hot.cbegin(); layer != out.hot.cend(); ++layer) {
        const qint64 hot = ExpertPlacer::hotBytes(layout, layer.key(), layer.value().size());
        hotTotal += hot;
        out.coldBytes += layout.expertBytes.value(layer.key()) - hot;
    }
    out.hotExpertBytes = hotTotal;
    out.gpuExpertBytes = hotTotal;
    for (const QVector<int> &chosen : placement.gpuExpertLayers) {
        QVector<int> kept;
        for (const int layer : chosen) {
            if (!out.hot.contains(layer)) {
                kept.append(layer);
                out.gpuExpertBytes += layout.expertBytes.value(layer);
            }
        }
        out.gpuExpertLayers.append(kept);
    }
    for (const int layer : placement.cpuExpertLayers) {
        if (!out.hot.contains(layer)) {
            out.cpuExpertLayers.append(layer);
        }
    }
}

} // namespace

Result<ExpertPlacement> ExpertPlacer::place(const ModelLayout &layout, const LayerPlacement &placement,
                                            const PerExpertRequest &request, const std::atomic<bool> *cancel) {
    if (!request.counts.ok()) {
        return Result<ExpertPlacement>::failure(request.counts.error());
    }
    const UsageCounts &counts = request.counts.value();
    ExpertPlacement out;
    const auto chosen = chooseHot(layout, counts, placement, request, cancel, out);
    if (!chosen.ok()) {
        return Result<ExpertPlacement>::failure(chosen.error());
    }
    out.hot = chosen.value().hot;
    applyHot(layout, placement, out);
    const double hitRate = HotBuilder::hitRate(layout, counts, out.hot, chosen.value().order);
    const auto isSplit = [&layout](const QVector<int> &experts) {
        return !experts.isEmpty() && experts.size() < layout.expertCount;
    };
    const auto splitCount = std::count_if(out.hot.cbegin(), out.hot.cend(), isSplit);
    // multi-argument arg(): a '%' in the hot file path must not be read as a placeholder
    out.reportLines.append(QStringLiteral("Per-expert placement: %1 layers in %2 (%3 split), %4 of hot experts; "
                                          "expected VRAM hit rate %5% (on the profile itself)")
                               .arg(QString::number(out.hot.size()), request.hotFilePath, QString::number(splitCount),
                                    gib(out.hotExpertBytes), fixed(100.0 * hitRate, 0)));
    return Result<ExpertPlacement>::success(out);
}

qint64 ExpertPlacer::largestColdSide(const ModelLayout &layout, const HotMap &hot) {
    qint64 largest = 0;
    for (auto layer = hot.cbegin(); layer != hot.cend(); ++layer) {
        const qsizetype count = layer.value().size();
        if (count > 0 && count < layout.expertCount) {
            const qint64 experts = layout.expertBytes.value(layer.key());
            largest = std::max(largest, experts * (layout.expertCount - count) / layout.expertCount);
        }
    }
    return largest;
}

qint64 ExpertPlacer::hotBytes(const ModelLayout &layout, int layer, qsizetype hotCount) {
    return layout.expertBytes.value(layer) * hotCount / layout.expertCount;
}

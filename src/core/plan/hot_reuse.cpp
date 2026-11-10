#include "plan/hot_reuse.h"

#include "common/constants.h"
#include "moe/hot_file.h"
#include "plan/expert_placer.h"

#include <algorithm>

namespace {

constexpr qint64 SPLIT_RESERVE_SLACK_BYTES = 256 * constants::MIB;

bool hasOnlyExpertLayers(const ModelLayout &layout, const HotMap &hot) {
    return std::all_of(hot.keyBegin(), hot.keyEnd(), [&layout](int layer) {
        return layer >= 0 && layer < layout.layerCount && layout.expertBytes.value(layer) != 0;
    });
}

// The first GPU's reserve: the fixed one without cost mode; in cost mode the largest cold side of
// a split layer (at least the fixed one), or nothing when no layer is split.
qint64 firstGpuReserve(const ModelLayout &layout, const HotMap &hot, bool isCostAware) {
    if (!isCostAware) {
        return constants::MOE_HOT_GPU0_EXTRA_BYTES;
    }
    const bool hasSplit = std::any_of(hot.cbegin(), hot.cend(), [&layout](const QVector<int> &experts) {
        return !experts.isEmpty() && experts.size() < layout.expertCount;
    });
    if (!hasSplit) {
        return 0;
    }
    return std::max(constants::MOE_HOT_GPU0_EXTRA_BYTES,
                    ExpertPlacer::largestColdSide(layout, hot) + SPLIT_RESERVE_SLACK_BYTES);
}

} // namespace

std::optional<HotMap> HotReuse::reuse(const ModelLayout &layout, const QByteArray &hotFileText,
                                      const LayerPlacement &placement, bool isCostAware) {
    const auto read = HotFile::read(hotFileText, layout.expertCount);
    if (!read.ok() || read.value().isEmpty() || !hasOnlyExpertLayers(layout, read.value())) {
        return std::nullopt;
    }
    const HotMap &hot = read.value();
    const qint64 reserve = firstGpuReserve(layout, hot, isCostAware);
    for (qsizetype gpu = 0; gpu < placement.layerCounts.size(); ++gpu) {
        qint64 used = 0;
        const int start = placement.rangeStarts[gpu];
        for (int layer = start; layer < start + placement.layerCounts[gpu]; ++layer) {
            if (hot.contains(layer)) {
                used += ExpertPlacer::hotBytes(layout, layer, hot.value(layer).size());
            }
        }
        if (used > placement.spareBytes[gpu] - (gpu == 0 ? reserve : 0)) {
            return std::nullopt;
        }
    }
    return hot;
}

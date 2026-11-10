#include "plan/placement_planner.h"

#include "common/constants.h"
#include "common/python_format.h"
#include "moe/hot_file.h"
#include "plan/expert_placer.h"
#include "plan/layer_placer.h"

#include <algorithm>
#include <numeric>

namespace {

using python_format::gib;

constexpr int LARGE_MICRO_BATCH = 2048;
constexpr int SMALL_MICRO_BATCH = 512;
// Generation speed follows the RAM-tier bytes, so 512 only wins when it shrinks that tier a lot.
constexpr double SMALL_BATCH_RAM_RATIO = 0.85;

Result<QVector<Gpu>> selectGpus(const QVector<int> &gpuIds, const HardwareSnapshot &hardware) {
    QVector<int> known;
    for (const Gpu &gpu : hardware.gpus) {
        known.append(gpu.index);
    }
    QVector<int> unknown;
    QVector<Gpu> selected;
    for (const int id : gpuIds) {
        const auto found = std::find_if(hardware.gpus.cbegin(), hardware.gpus.cend(), [id](const Gpu &gpu) { return gpu.index == id; });
        found == hardware.gpus.cend() ? unknown.append(id) : selected.append(*found);
    }
    if (!unknown.isEmpty()) {
        std::sort(known.begin(), known.end());
        return Result<QVector<Gpu>>::failure(QStringLiteral("GPU id(s) %1 not found; nvidia-smi reports %2")
                                                 .arg(python_format::intList(unknown), python_format::intList(known)));
    }
    if (selected.isEmpty()) {
        return Result<QVector<Gpu>>::failure(QStringLiteral("no GPUs selected"));
    }
    return Result<QVector<Gpu>>::success(selected);
}

qint64 sum(const QVector<qint64> &values) {
    return std::accumulate(values.cbegin(), values.cend(), qint64(0));
}

QString layerRegex(QVector<int> layers) {
    std::sort(layers.begin(), layers.end());
    QStringList parts;
    for (const int layer : layers) {
        parts.append(QString::number(layer));
    }
    return QStringLiteral("blk\\.(%1)\\.ffn_.*exps").arg(parts.join(u'|'));
}

QStringList serverArgs(const PlanRequest &request, const Plan &plan, int gpuCount) {
    QStringList args{QStringLiteral("-ngl"), QStringLiteral("999"), QStringLiteral("-c"), QString::number(request.contextSize),
                     QStringLiteral("-b"), QString::number(std::max(LARGE_MICRO_BATCH, plan.microBatch)),
                     QStringLiteral("-ub"), QString::number(plan.microBatch)};
    if (gpuCount > 1) {
        // llama.cpp splits block_count + 1 (output) layers; count the output and skipped MTP layers
        // on the last GPU so the boundaries land exactly where the plan put them
        QStringList split;
        for (int gpu = 0; gpu < gpuCount; ++gpu) {
            const bool isLast = gpu == gpuCount - 1;
            split.append(QString::number(plan.gpuLayerCounts[gpu] + (isLast ? request.layout.skippedLayerCount + 1 : 0)));
        }
        args << QStringLiteral("-ts") << split.join(u',');
    }
    // one comma-separated -ot: newer llama.cpp keeps only the last -ot; first match wins
    QStringList overrides;
    for (int gpu = 0; gpu < plan.gpuExpertLayers.size(); ++gpu) {
        if (!plan.gpuExpertLayers[gpu].isEmpty()) {
            overrides.append(QStringLiteral("%1=CUDA%2").arg(layerRegex(plan.gpuExpertLayers[gpu])).arg(gpu));
        }
    }
    if (!plan.cpuExpertLayers.isEmpty()) {
        overrides.append(QStringLiteral("ffn_.*exps=CPU"));
    }
    if (!overrides.isEmpty()) {
        args << QStringLiteral("-ot") << overrides.join(u',');
    }
    if (!plan.hotFilePath.isEmpty()) {
        // the layers in the file ignore -ot, so the catch-all above can not move their hot experts
        args << QStringLiteral("--moe-hot") << plan.hotFilePath;
    }
    return args;
}

void decideRamTier(const PlanRequest &request, const HardwareSnapshot &hardware, Plan &plan) {
    const qint64 needed = plan.ramTierBytes;
    const auto &ram = hardware.freeRamBytes;
    // Loading the RAM tier up front measured ~2x faster than mmap paging it in, so only stream
    // from the SSD when the RAM tier does not fit (keeping some RAM free for the OS).
    const bool isFitting = ram.has_value() && needed + constants::RAM_SPARE_BYTES <= *ram;
    plan.useMmap = request.allowSsd && !isFitting;
    if (!ram) {
        plan.report.append(QStringLiteral("RAM: %1 needed, free RAM unknown").arg(gib(needed)));
    } else if (isFitting) {
        plan.report.append(QStringLiteral("RAM: %1 needed, %2 free -> loaded into RAM up front (no SSD streaming)").arg(gib(needed), gib(*ram)));
    } else if (request.allowSsd) {
        plan.warnings.append(QStringLiteral("RAM tier (%1) does not fit in free RAM (%2): streaming the rest from the SSD (mmap). "
                                            "Expect it to be slower, and first replies slowest.").arg(gib(needed), gib(*ram)));
    } else {
        plan.warnings.append(QStringLiteral("RAM tier (%1) does not fit in free RAM (%2) and SSD streaming is off: "
                                            "loading will likely fail or swap heavily.").arg(gib(needed), gib(*ram)));
    }
}

} // namespace

// The per-expert branch replaces the whole-layer lists for the layers the hot file names.
Result<qint64> applyPerExpert(const PlanRequest &request, const LayerPlacement &placed, const std::atomic<bool> *cancel,
                              Plan &plan) {
    const PerExpertRequest &perExpert = *request.perExpert;
    const auto experts = ExpertPlacer::place(request.layout, placed, perExpert, cancel);
    if (!experts.ok()) {
        return Result<qint64>::failure(experts.error());
    }
    const ExpertPlacement &placement = experts.value();
    plan.report += placement.reportLines;
    plan.gpuExpertLayers = placement.gpuExpertLayers;
    plan.cpuExpertLayers = placement.cpuExpertLayers;
    plan.hotExperts = placement.hot;
    plan.hotFilePath = perExpert.hotFilePath;
    plan.isHotFileReused = placement.isReused;
    plan.hotFileText = placement.isReused ? *perExpert.cachedHotFile
                                          : HotFile::write(request.layout.expertCount, placement.hot);
    plan.ramTierBytes += placement.coldBytes;
    return Result<qint64>::success(placement.gpuExpertBytes);
}

Result<Plan> PlacementPlanner::plan(const PlanRequest &request, const HardwareSnapshot &hardware,
                                    const std::atomic<bool> *cancel) {
    const ModelLayout &layout = request.layout;
    Plan plan;
    plan.arch = layout.arch;
    plan.report.append(QStringLiteral("Model: %1, %2 layers, %3 experts (%4 active), %5 on disk")
                           .arg(layout.arch).arg(layout.layerCount).arg(layout.expertCount).arg(layout.expertUsedCount).arg(gib(layout.totalBytes)));
    const qint64 totalExpertBytes = sum(layout.expertBytes);
    if (totalExpertBytes == 0) {
        plan.warnings.append(QStringLiteral("No MoE expert tensors found: this is a dense model, so only layer offload applies."));
    }
    const auto gpus = selectGpus(request.gpuIds, hardware);
    if (!gpus.ok()) {
        return Result<Plan>::failure(gpus.error());
    }
    QVector<qint64> kvBytes;
    for (const qint64 perToken : layout.kvBytesPerToken) {
        kvBytes.append(perToken * request.contextSize);
    }
    const auto placeWith = [&](int microBatch) {
        return LayerPlacer::place(layout, gpus.value(), kvBytes, request.marginMib, microBatch, request.reserveLastBytes, request.contextSize);
    };
    plan.microBatch = request.microBatch.value_or(LARGE_MICRO_BATCH);
    auto placement = placeWith(plan.microBatch);
    if (!placement.ok()) {
        return Result<Plan>::failure(placement.error());
    }
    if (!request.microBatch) {
        if (!placement.value().cpuExpertLayers.isEmpty()) {
            auto small = placeWith(SMALL_MICRO_BATCH);
            if (small.ok() && totalExpertBytes - small.value().gpuExpertBytes <=
                                  double(totalExpertBytes - placement.value().gpuExpertBytes) * SMALL_BATCH_RAM_RATIO) {
                placement = std::move(small);
                plan.microBatch = SMALL_MICRO_BATCH;
            }
        }
        plan.report.append(QStringLiteral("Micro-batch: auto -> %1").arg(plan.microBatch) +
                           (plan.microBatch == SMALL_MICRO_BATCH ? QStringLiteral(" (frees VRAM for experts; prompts read a bit slower)") : QString()));
    }
    const LayerPlacement &placed = placement.value();
    plan.report += placed.lines;
    plan.warnings += placed.warnings;
    plan.gpuLayerCounts = placed.layerCounts;
    plan.gpuExpertLayers = placed.gpuExpertLayers;
    plan.cpuExpertLayers = placed.cpuExpertLayers;

    qint64 gpuExpertBytes = placed.gpuExpertBytes;
    if (request.perExpert) {
        const auto perExpert = applyPerExpert(request, placed, cancel, plan);
        if (!perExpert.ok()) {
            return Result<Plan>::failure(perExpert.error());
        }
        gpuExpertBytes = perExpert.value();
    }
    plan.ramTierBytes += layout.cpuGlobalBytes;
    for (const int layer : plan.cpuExpertLayers) {
        plan.ramTierBytes += layout.expertBytes[layer];
    }
    // 100 * int / int in Python: exact integer product, then one correctly rounded division
    const double vramShare = double(100 * gpuExpertBytes) / double(totalExpertBytes != 0 ? totalExpertBytes : 1);
    plan.report.append(QStringLiteral("Experts: %1% in VRAM, %2 layer(s) / %3 in RAM")
                           .arg(python_format::fixed(vramShare, 0)).arg(plan.cpuExpertLayers.size()).arg(gib(plan.ramTierBytes)));
    decideRamTier(request, hardware, plan);

    plan.args = serverArgs(request, plan, int(gpus.value().size()));
    if (!layout.stopTokenOverrides.isEmpty()) {
        plan.report.append(QStringLiteral("Stop tokens: model marks only <|endoftext|> as EOS; also stopping on <|user|> / <|observation|>"));
        for (const QString &fix : layout.stopTokenOverrides) {
            plan.args << QStringLiteral("--override-kv") << fix;
        }
    }
    return Result<Plan>::success(plan);
}

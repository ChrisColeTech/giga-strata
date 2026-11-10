#include "plan_tiers.h"

#include "common/constants.h"
#include "plan/buffer_model.h"
#include "plan/expert_placer.h"

#include <QCoreApplication>
#include <QVariantMap>

#include <algorithm>

namespace {

enum class Home { Gpu, Ram, Split };

// Where a layer's experts live; attention and KV are always on the layer's GPU.
Home homeOf(const ModelLayout &layout, const Plan &plan, int layer) {
    if (layout.expertBytes.value(layer) == 0) {
        return Home::Gpu;
    }
    if (plan.hotExperts.contains(layer)) {
        const qsizetype hot = plan.hotExperts.value(layer).size();
        return hot == layout.expertCount ? Home::Gpu : hot == 0 ? Home::Ram : Home::Split;
    }
    return plan.cpuExpertLayers.contains(layer) ? Home::Ram : Home::Gpu;
}

QString gpuLabel(const Gpu &gpu) {
    return QStringLiteral("GPU%1").arg(gpu.index);
}

QVariantMap tier(const QString &label, int index, qint64 used, qint64 capacity, const QString &layerText) {
    return {{QStringLiteral("label"), label},
            {QStringLiteral("tierIndex"), index},
            {QStringLiteral("usedBytes"), double(used)},
            {QStringLiteral("capacityBytes"), double(capacity)},
            {QStringLiteral("layerText"), layerText}};
}

QString translated(const char *text) {
    return QCoreApplication::translate("PlanTiers", text);
}

qint64 gpuBytes(const PlanTiers::Input &input, int gpu, int start, int count) {
    const ModelLayout &layout = *input.layout;
    const bool isLast = gpu == input.gpus.size() - 1;
    qint64 used = BufferModel::computeBufferBytes(layout, input.plan->microBatch, isLast, input.contextSize);
    used += isLast ? layout.outputBytes + input.reserveLastBytes : 0;
    for (int layer = start; layer < start + count; ++layer) {
        used += layout.denseBytes.value(layer) + layout.kvBytesPerToken.value(layer) * input.contextSize;
        if (input.plan->hotExperts.contains(layer)) {
            used += ExpertPlacer::hotBytes(layout, layer, input.plan->hotExperts.value(layer).size());
        } else if (homeOf(layout, *input.plan, layer) == Home::Gpu) {
            used += layout.expertBytes.value(layer);
        }
    }
    return used;
}

} // namespace

QVariantList PlanTiers::tiers(const Input &input) {
    QVariantList result;
    const Plan &plan = *input.plan;
    int start = 0;
    for (int gpu = 0; gpu < input.gpus.size() && gpu < plan.gpuLayerCounts.size(); ++gpu) {
        const int count = plan.gpuLayerCounts[gpu];
        const QString layers = count > 0 ? translated("layers %1-%2").arg(start).arg(start + count - 1) : QString();
        result.append(tier(gpuLabel(input.gpus[gpu]), gpu, gpuBytes(input, gpu, start, count),
                           input.gpus[gpu].freeMib * constants::MIB, layers));
        start += count;
    }
    const qint64 ramTier = plan.ramTierBytes;
    qint64 inRam = ramTier;
    if (plan.useMmap && input.freeRamBytes) {
        inRam = std::clamp<qint64>(*input.freeRamBytes - constants::RAM_SPARE_BYTES, 0, ramTier);
    }
    int ramLayers = 0;
    for (int layer = 0; layer < input.layout->layerCount; ++layer) {
        ramLayers += homeOf(*input.layout, plan, layer) == Home::Ram ? 1 : 0;
    }
    result.append(tier(QStringLiteral("RAM"), int(input.gpus.size()), inRam, input.freeRamBytes.value_or(0),
                       translated("experts of %1 layer(s)").arg(ramLayers)));
    if (inRam < ramTier) {
        result.append(tier(QStringLiteral("SSD"), int(input.gpus.size()) + 1, ramTier - inRam, 0,
                           translated("streamed from the SSD")));
    }
    return result;
}

QString PlanTiers::summary(const Input &input) {
    const ModelLayout &layout = *input.layout;
    const Plan &plan = *input.plan;
    QStringList parts;
    int start = 0;
    int ram = 0;
    int split = 0;
    for (int gpu = 0; gpu < input.gpus.size() && gpu < plan.gpuLayerCounts.size(); ++gpu) {
        int onGpu = 0;
        for (int layer = start; layer < start + plan.gpuLayerCounts[gpu]; ++layer) {
            const Home home = homeOf(layout, plan, layer);
            onGpu += home == Home::Gpu ? 1 : 0;
            ram += home == Home::Ram ? 1 : 0;
            split += home == Home::Split ? 1 : 0;
        }
        parts.append(translated("%1 on %2").arg(onGpu).arg(gpuLabel(input.gpus[gpu])));
        start += plan.gpuLayerCounts[gpu];
    }
    if (ram > 0) {
        parts.append(translated("%1 in RAM").arg(ram));
    }
    if (split > 0) {
        parts.append(translated("%1 split").arg(split));
    }
    return translated("%1 layers: %2").arg(layout.layerCount).arg(parts.join(QStringLiteral(", ")));
}

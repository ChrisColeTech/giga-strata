#pragma once

#include "gguf/model_layout.h"
#include "moe/hot_map.h"
#include "plan/layer_placement.h"

#include <QByteArray>

#include <optional>

// strata_planner._reuse_hot: the experts in an existing --moe-hot file, if they still fit each
// GPU's spare VRAM in this placement. Uses the same first-GPU reserve as building does.
class HotReuse {
public:
    static std::optional<HotMap> reuse(const ModelLayout &layout, const QByteArray &hotFileText,
                                       const LayerPlacement &placement, bool isCostAware);
};

#pragma once

#include "hardware/gpu.h"

#include <QVector>

#include <optional>

// Hardware state as one immutable value: the planner reads this instead of querying anything,
// so goldens inject it and live re-plans never mix two readings (02 sections 5 and 8).
struct HardwareSnapshot {
    QVector<Gpu> gpus;
    std::optional<qint64> freeRamBytes; // nullopt when free RAM could not be read
};

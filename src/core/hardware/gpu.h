#pragma once

#include <QString>

// strata_planner.Gpu: one row of nvidia-smi.
struct Gpu {
    int index = 0;        // nvidia-smi / PCI bus order index
    QString name;
    qint64 freeMib = 0;
    qint64 totalMib = 0;
};

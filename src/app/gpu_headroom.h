#pragma once

#include "hardware/hardware_snapshot.h"

#include <QMap>
#include <QString>

// GPU headroom: VRAM kept free on a GPU for other programs, on top of the planner's margin.
// Stored as text "<nvidia-smi index>:<GB>,..." ("0:4,1:0.5"), app-wide in Settings > Server and
// per model in its parameters; a model's entry for a GPU replaces the app-wide one.
class GpuHeadroom {
public:
    // Entries that are not "<index>:<GB>" with a non-negative GB are skipped.
    static QMap<int, double> parse(const QString &text);
    static QString format(const QMap<int, double> &gigabytes);

    // The app-wide entries with the model's entries laid over them.
    static QString merged(const QString &appWide, const QString &model);

    // Lowers each GPU's free VRAM by its headroom, never below zero.
    static void apply(HardwareSnapshot &hardware, const QString &text);
};

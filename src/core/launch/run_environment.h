#pragma once

#include "hardware/hardware_snapshot.h"

#include <QSet>
#include <QString>

class ModelSource;
class UsageSource;

// The world one Run is planned against: what the files say (through the injected sources, so
// goldens can serve fixtures), the hardware reading, the build's flags and the data folders.
struct RunEnvironment {
    const ModelSource *models = nullptr;
    const UsageSource *usage = nullptr;
    HardwareSnapshot hardware;
    QSet<QString> flags;            // detect_flags of the binary that will run
    QString modelsDirectory;        // [MODELS_DIR] extra draft and projector search folder
    QString usageDirectory;         // [USAGE_DIR]
    QString hotDirectory;           // [HOT_DIR]
};

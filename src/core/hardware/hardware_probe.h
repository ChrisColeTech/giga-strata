#pragma once

#include "common/result.h"
#include "hardware/gpu.h"
#include "hardware/hardware_snapshot.h"

#include <QString>
#include <QVector>

// The only code that runs nvidia-smi (02 section 2). Blocking: callers run it on a worker
// thread (R27). The program is injectable so tests can point it at something that fails.
class HardwareProbe {
public:
    static constexpr int TIMEOUT_MS = 5000;

    explicit HardwareProbe(QString program = QStringLiteral("nvidia-smi"));

    Result<QVector<Gpu>> queryGpus() const;
    // GPUs are empty when nvidia-smi fails; queryGpus() says why.
    HardwareSnapshot snapshot() const;

private:
    QString m_program;
};

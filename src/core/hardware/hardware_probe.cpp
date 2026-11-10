#include "hardware/hardware_probe.h"

#include "hardware/gpu_info.h"
#include "hardware/ram_info.h"

#include <QCoreApplication>
#include <QProcess>

#include <utility>

#include <windows.h>

namespace {

QString translated(const char *text) {
    return QCoreApplication::translate("HardwareProbe", text);
}

} // namespace

HardwareProbe::HardwareProbe(QString program) : m_program(std::move(program)) {}

Result<QVector<Gpu>> HardwareProbe::queryGpus() const {
    QProcess process;
    // Same as Python's CREATE_NO_WINDOW: polling must not flash a console every second.
    process.setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments *arguments) { arguments->flags |= CREATE_NO_WINDOW; });
    process.start(m_program, GpuInfo::nvidiaSmiArguments());
    if (!process.waitForStarted(TIMEOUT_MS)) {
        return Result<QVector<Gpu>>::failure(
            translated("nvidia-smi was not found. Install the NVIDIA driver to see GPU memory."));
    }
    if (!process.waitForFinished(TIMEOUT_MS)) {
        process.kill();
        process.waitForFinished();
        return Result<QVector<Gpu>>::failure(translated("nvidia-smi did not answer within 5 seconds."));
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return Result<QVector<Gpu>>::failure(
            translated("nvidia-smi failed with exit code %1. Check the NVIDIA driver.").arg(process.exitCode()));
    }
    return GpuInfo::parseCsv(QString::fromLocal8Bit(process.readAllStandardOutput()));
}

HardwareSnapshot HardwareProbe::snapshot() const {
    HardwareSnapshot snapshot;
    const Result<QVector<Gpu>> gpus = queryGpus();
    if (gpus.ok()) {
        snapshot.gpus = gpus.value();
    }
    snapshot.freeRamBytes = RamInfo::freeBytes();
    return snapshot;
}

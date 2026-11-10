#include "monitor_controller.h"

#include "hardware/gpu_info.h"
#include "hardware/ram_info.h"

#include <QJSEngine>
#include <QVariantMap>
#include <QtConcurrent/QtConcurrentRun>

#include <utility>

MonitorController::MonitorController(HardwareProbe probe, QObject *parent) : QObject(parent), m_probe(std::move(probe)) {
    m_timer.setInterval(POLL_INTERVAL_MS);
    connect(&m_timer, &QTimer::timeout, this, &MonitorController::poll);
    connect(&m_watcher, &QFutureWatcher<Reading>::finished, this, &MonitorController::applyReading);
    m_timer.start();
    poll();
}

MonitorController::~MonitorController() {
    stop();
}

void MonitorController::provide(MonitorController *instance) {
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

MonitorController *MonitorController::create(QQmlEngine *, QJSEngine *) {
    Q_ASSERT_X(s_instance, "MonitorController::create",
               "main.cpp must call MonitorController::provide() before loading QML");
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

QVariantList MonitorController::gpus() const {
    QVariantList list;
    for (const Gpu &gpu : m_reading.gpus) {
        list.append(QVariantMap{
            {QStringLiteral("index"), gpu.index},
            {QStringLiteral("name"), gpu.name},
            {QStringLiteral("shortName"), GpuInfo::shortName(gpu.name)},
            {QStringLiteral("freeMib"), static_cast<double>(gpu.freeMib)},
            {QStringLiteral("totalMib"), static_cast<double>(gpu.totalMib)},
        });
    }
    return list;
}

std::optional<HardwareSnapshot> MonitorController::snapshot() const {
    if (!m_hasReading) {
        return std::nullopt;
    }
    return HardwareSnapshot{m_reading.gpus, m_reading.freeRamBytes};
}

void MonitorController::setPaused(bool isPaused) {
    if (m_isPaused == isPaused || m_isStopped) {
        return;
    }
    m_isPaused = isPaused;
    if (m_isPaused) {
        m_timer.stop();
    } else {
        m_timer.start();
        poll();
    }
    emit pausedChanged();
}

void MonitorController::stop() {
    m_isStopped = true;
    m_timer.stop();
    m_watcher.waitForFinished();
}

void MonitorController::poll() {
    if (m_isStopped || m_watcher.isRunning()) {
        return;
    }
    const HardwareProbe probe = m_probe;
    m_watcher.setFuture(QtConcurrent::run([probe] {
        Reading reading;
        const Result<QVector<Gpu>> gpus = probe.queryGpus();
        if (gpus.ok()) {
            reading.gpus = gpus.value();
        } else {
            reading.gpuError = gpus.error();
        }
        reading.freeRamBytes = RamInfo::freeBytes();
        reading.totalRamBytes = RamInfo::totalBytes();
        return reading;
    }));
}

void MonitorController::applyReading() {
    if (m_isStopped) {
        return;
    }
    m_reading = m_watcher.result();
    m_hasReading = true;
    emit readingChanged();
}

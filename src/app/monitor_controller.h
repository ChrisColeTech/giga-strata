#pragma once

#include "hardware/hardware_probe.h"

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <optional>

class QJSEngine;
class QQmlEngine;

// Live GPU and RAM readings for the chips and gauges (FR-4.2). Polls every second on a worker
// (R27); pauses while the window is minimized or hidden and stops for good on quit (R28). A poll
// never overlaps the previous one, so a slow nvidia-smi cannot pile up processes.
class MonitorController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList gpus READ gpus NOTIFY readingChanged)
    Q_PROPERTY(QString gpuError READ gpuError NOTIFY readingChanged)
    Q_PROPERTY(bool hasReading READ hasReading NOTIFY readingChanged)
    Q_PROPERTY(bool hasRam READ hasRam NOTIFY readingChanged)
    Q_PROPERTY(double freeRamBytes READ freeRamBytes NOTIFY readingChanged)
    Q_PROPERTY(double totalRamBytes READ totalRamBytes NOTIFY readingChanged)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY pausedChanged)

public:
    static constexpr int POLL_INTERVAL_MS = 1000;

    explicit MonitorController(HardwareProbe probe, QObject *parent = nullptr);
    ~MonitorController() override;

    static void provide(MonitorController *instance);
    static MonitorController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    QVariantList gpus() const;
    const QString &gpuError() const { return m_reading.gpuError; }
    bool hasReading() const { return m_hasReading; }
    bool hasRam() const { return m_reading.freeRamBytes.has_value() && m_reading.totalRamBytes.has_value(); }
    double freeRamBytes() const { return static_cast<double>(m_reading.freeRamBytes.value_or(0)); }
    double totalRamBytes() const { return static_cast<double>(m_reading.totalRamBytes.value_or(0)); }
    bool isPaused() const { return m_isPaused; }
    // The latest reading as the planner's input (05 section 3); nullopt before the first one.
    std::optional<HardwareSnapshot> snapshot() const;

public slots:
    void setPaused(bool isPaused);
    void stop();

signals:
    void readingChanged();
    void pausedChanged();

private:
    struct Reading {
        QVector<Gpu> gpus;
        QString gpuError;
        std::optional<qint64> freeRamBytes;
        std::optional<qint64> totalRamBytes;
    };

    void poll();
    void applyReading();

    static inline MonitorController *s_instance = nullptr;

    HardwareProbe m_probe;
    QTimer m_timer;
    QFutureWatcher<Reading> m_watcher;
    Reading m_reading;
    bool m_hasReading = false;
    bool m_isPaused = false;
    bool m_isStopped = false;
};

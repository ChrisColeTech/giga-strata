#include "monitor_controller.h"

#include <QSignalSpy>
#include <QtTest>

// Uses a probe whose nvidia-smi does not exist, so no GPU is touched and the run is offline.
class TestMonitorController : public QObject {
    Q_OBJECT

private slots:
    void firstReadingCarriesRamAndGpuError() {
        MonitorController monitor(HardwareProbe(QStringLiteral("giga-strata-no-such-nvidia-smi")));
        QSignalSpy readingSpy(&monitor, &MonitorController::readingChanged);
        QVERIFY(readingSpy.wait(MonitorController::POLL_INTERVAL_MS * 3));
        QVERIFY(monitor.hasReading());
        QVERIFY(monitor.hasRam());
        QVERIFY(monitor.freeRamBytes() <= monitor.totalRamBytes());
        QVERIFY(monitor.gpus().isEmpty());
        QVERIFY(!monitor.gpuError().isEmpty());
    }

    void pausedMonitorStopsPolling() {
        MonitorController monitor(HardwareProbe(QStringLiteral("giga-strata-no-such-nvidia-smi")));
        QSignalSpy readingSpy(&monitor, &MonitorController::readingChanged);
        QVERIFY(readingSpy.wait(MonitorController::POLL_INTERVAL_MS * 3));
        monitor.setPaused(true);
        QVERIFY(monitor.isPaused());
        readingSpy.clear();
        QTest::qWait(MonitorController::POLL_INTERVAL_MS + MonitorController::POLL_INTERVAL_MS / 2);
        QCOMPARE(readingSpy.count(), 0);
    }

    void stoppedMonitorIgnoresResume() {
        MonitorController monitor(HardwareProbe(QStringLiteral("giga-strata-no-such-nvidia-smi")));
        monitor.stop();
        monitor.setPaused(false);
        monitor.setPaused(true);
        QVERIFY(!monitor.isPaused());
    }
};

QTEST_GUILESS_MAIN(TestMonitorController)
#include "test_monitor_controller.moc"

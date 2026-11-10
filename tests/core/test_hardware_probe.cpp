#include "hardware/hardware_probe.h"

#include <QtTest>

// Offline by design (R31): only the failure path runs, so the test passes on a machine without
// NVIDIA GPUs and never touches the GPUs that are busy on this one.
class TestHardwareProbe : public QObject {
    Q_OBJECT

private slots:
    void missingProgramFailsWithSentence() {
        const HardwareProbe probe(QStringLiteral("giga-strata-no-such-nvidia-smi"));
        const Result<QVector<Gpu>> gpus = probe.queryGpus();
        QVERIFY(!gpus.ok());
        QVERIFY(gpus.error().contains(QStringLiteral("nvidia-smi was not found")));
    }

    void snapshotKeepsRamWhenGpusFail() {
        const HardwareProbe probe(QStringLiteral("giga-strata-no-such-nvidia-smi"));
        const HardwareSnapshot snapshot = probe.snapshot();
        QVERIFY(snapshot.gpus.isEmpty());
        QVERIFY(snapshot.freeRamBytes.has_value());
    }
};

QTEST_GUILESS_MAIN(TestHardwareProbe)
#include "test_hardware_probe.moc"

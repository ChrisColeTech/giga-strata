#include "gpu_headroom.h"

#include <QtTest>

class TestGpuHeadroom : public QObject {
    Q_OBJECT

private slots:
    void parsesAndSkipsBadEntries() {
        const QMap<int, double> parsed = GpuHeadroom::parse(QStringLiteral(" 0:4, 1:0.5,x:2,2:-1,3:,4"));
        QCOMPARE(parsed.size(), 2);
        QCOMPARE(parsed.value(0), 4.0);
        QCOMPARE(parsed.value(1), 0.5);
        QCOMPARE(GpuHeadroom::format(parsed), QStringLiteral("0:4,1:0.5"));
    }

    void modelEntriesReplaceAppWideOnes() {
        QCOMPARE(GpuHeadroom::merged(QStringLiteral("0:2,1:2"), QStringLiteral("1:6")), QStringLiteral("0:2,1:6"));
        QCOMPARE(GpuHeadroom::merged(QString(), QStringLiteral("1:0")), QStringLiteral("1:0"));
        QCOMPARE(GpuHeadroom::merged(QStringLiteral("0:3"), QString()), QStringLiteral("0:3"));
    }

    void lowersFreeVramPerGpuNeverBelowZero() {
        HardwareSnapshot hardware{{Gpu{0, QStringLiteral("A"), 10240, 32768}, Gpu{1, QStringLiteral("B"), 1024, 32768},
                                   Gpu{2, QStringLiteral("C"), 4096, 32768}},
                                  std::nullopt};
        GpuHeadroom::apply(hardware, QStringLiteral("0:1.5,1:4"));
        QCOMPARE(hardware.gpus[0].freeMib, qint64(10240 - 1536));
        QCOMPARE(hardware.gpus[1].freeMib, qint64(0));
        QCOMPARE(hardware.gpus[2].freeMib, qint64(4096));
    }
};

QTEST_APPLESS_MAIN(TestGpuHeadroom)
#include "test_gpu_headroom.moc"

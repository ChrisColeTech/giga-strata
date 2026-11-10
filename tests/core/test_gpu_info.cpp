#include "hardware/gpu_info.h"

#include <QtTest>

namespace {

// Captured from this machine's nvidia-smi (2x RTX 5090) on 2026-11-10, CRLF line ends included.
const QString CAPTURED_CSV = QStringLiteral("0, NVIDIA GeForce RTX 5090, 31833, 32607\r\n"
                                            "1, NVIDIA GeForce RTX 5090, 32187, 32607\r\n");

} // namespace

class TestGpuInfo : public QObject {
    Q_OBJECT

private slots:
    void argumentsMatchPythonQuery() {
        QCOMPARE(GpuInfo::nvidiaSmiArguments(),
                 QStringList({QStringLiteral("--query-gpu=index,name,memory.free,memory.total"),
                              QStringLiteral("--format=csv,noheader,nounits")}));
    }

    void parsesCapturedOutput() {
        const Result<QVector<Gpu>> gpus = GpuInfo::parseCsv(CAPTURED_CSV);
        QVERIFY(gpus.ok());
        QCOMPARE(gpus.value().size(), 2);
        QCOMPARE(gpus.value()[0].index, 0);
        QCOMPARE(gpus.value()[0].name, QStringLiteral("NVIDIA GeForce RTX 5090"));
        QCOMPARE(gpus.value()[0].freeMib, 31833);
        QCOMPARE(gpus.value()[0].totalMib, 32607);
        QCOMPARE(gpus.value()[1].index, 1);
        QCOMPARE(gpus.value()[1].freeMib, 32187);
    }

    void emptyOutputMeansNoGpus() {
        const Result<QVector<Gpu>> gpus = GpuInfo::parseCsv(QStringLiteral("\r\n"));
        QVERIFY(gpus.ok());
        QVERIFY(gpus.value().isEmpty());
    }

    void malformedRowFailsWithSentence() {
        const Result<QVector<Gpu>> notAvailable =
            GpuInfo::parseCsv(QStringLiteral("0, NVIDIA GeForce RTX 5090, [N/A], 32607"));
        QVERIFY(!notAvailable.ok());
        QVERIFY(notAvailable.error().contains(QStringLiteral("[N/A]")));
        QVERIFY(!GpuInfo::parseCsv(QStringLiteral("0, RTX, 1")).ok());
    }

    void shortNameDropsVendorPrefixes() {
        QCOMPARE(GpuInfo::shortName(QStringLiteral("NVIDIA GeForce RTX 5090")), QStringLiteral("RTX 5090"));
        QCOMPARE(GpuInfo::shortName(QStringLiteral("NVIDIA RTX A6000")), QStringLiteral("RTX A6000"));
        QCOMPARE(GpuInfo::shortName(QStringLiteral("Tesla T4")), QStringLiteral("Tesla T4"));
    }
};

QTEST_APPLESS_MAIN(TestGpuInfo)
#include "test_gpu_info.moc"

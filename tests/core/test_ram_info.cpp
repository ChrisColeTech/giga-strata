#include "hardware/ram_info.h"

#include <QtTest>

class TestRamInfo : public QObject {
    Q_OBJECT

private slots:
    void freeIsPositiveAndWithinTotal() {
        const std::optional<qint64> freeBytes = RamInfo::freeBytes();
        const std::optional<qint64> totalBytes = RamInfo::totalBytes();
        QVERIFY(freeBytes.has_value());
        QVERIFY(totalBytes.has_value());
        QVERIFY(*freeBytes > 0);
        QVERIFY(*freeBytes <= *totalBytes);
    }
};

QTEST_APPLESS_MAIN(TestRamInfo)
#include "test_ram_info.moc"

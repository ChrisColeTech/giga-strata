#include "common/result.h"

#include <QtTest>

class TestResult : public QObject {
    Q_OBJECT

private slots:
    void successCarriesValue() {
        const auto result = Result<int>::success(42);
        QVERIFY(result.ok());
        QCOMPARE(result.value(), 42);
        QVERIFY(result.error().isEmpty());
    }

    void failureCarriesSentence() {
        const auto result = Result<int>::failure(QStringLiteral("No GPUs selected."));
        QVERIFY(!result.ok());
        QCOMPARE(result.error(), QStringLiteral("No GPUs selected."));
    }

    void voidResult() {
        QVERIFY(Result<void>::success().ok());
        const auto failed = Result<void>::failure(QStringLiteral("Disk full."));
        QVERIFY(!failed.ok());
        QCOMPARE(failed.error(), QStringLiteral("Disk full."));
    }
};

QTEST_APPLESS_MAIN(TestResult)
#include "test_result.moc"

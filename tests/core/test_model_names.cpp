#include "common/model_names.h"

#include "golden_cases.h"

#include <QtTest>

class TestModelNames : public QObject {
    Q_OBJECT

private slots:
    void stemMatchesTheReference() {
        const auto cases = GoldenCases::small(QStringLiteral("model_stem"));
        QVERIFY(!cases.isEmpty());
        for (const auto &[input, expected] : cases) {
            QCOMPARE(ModelNames::modelStem(input.value(QStringLiteral("path")).toString()),
                     expected.value(QStringLiteral("stem")).toString());
        }
    }

    void mtimeSecondsFollowPythonFloat() {
        // sec + nsec * 1e-9 rounds up here (golden README section 8 item 1).
        QCOMPARE(ModelNames::pythonMtimeSeconds(1793757055999999900LL), qint64(1793757056));
        QCOMPARE(ModelNames::pythonMtimeSeconds(1793757055123456700LL), qint64(1793757055));
        QCOMPARE(ModelNames::pythonMtimeSeconds(-1500000000LL), qint64(-1));
        QCOMPARE(ModelNames::pythonMtimeSeconds(0), qint64(0));
    }

    void keyUsesNormcasedAbsolutePath() {
        QCOMPARE(ModelNames::modelKey(QStringLiteral("N:/Models/A.gguf"), 12, 1999999999LL),
                 QStringLiteral("n:\\models\\a.gguf|12|1"));
    }

    void usagePathAndBaseName() {
        QCOMPARE(ModelNames::usagePath(QStringLiteral("moe_usage"), QStringLiteral("N:\\m\\Q-00001-of-00002.gguf")),
                 QStringLiteral("moe_usage\\Q.usage.json"));
        QCOMPARE(ModelNames::baseName(QStringLiteral("C:model.gguf")), QStringLiteral("model.gguf"));
        QCOMPARE(ModelNames::normcase(QStringLiteral("A/B\\C")), QStringLiteral("a\\b\\c"));
    }
};

QTEST_APPLESS_MAIN(TestModelNames)
#include "test_model_names.moc"

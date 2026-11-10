#include "launch/flag_probe.h"

#include "fake_server_environment.h"
#include "golden_cases.h"

#include <QFile>
#include <QtTest>

class TestFlagProbe : public QObject {
    Q_OBJECT

private slots:
    void scansTheCapturedHelpTexts() {
        const QJsonArray builds =
            GoldenCases::readObject(GoldenCases::root() + QStringLiteral("/fixtures/help/index.json"))
                .value(QStringLiteral("builds"))
                .toArray();
        QCOMPARE(builds.size(), 3);
        for (const QJsonValue &value : builds) {
            const QJsonObject build = value.toObject();
            QFile help(GoldenCases::root() + QStringLiteral("/fixtures/help/") +
                       build.value(QStringLiteral("file")).toString());
            QVERIFY(help.open(QIODevice::ReadOnly));
            const QString id = build.value(QStringLiteral("id")).toString();
            const QString expectedPath = GoldenCases::root() + QStringLiteral("/expected/help/") + id;
            const QJsonObject expected = GoldenCases::readObject(expectedPath + QStringLiteral(".json"));
            QSet<QString> flags;
            for (const QJsonValue &flag : expected.value(QStringLiteral("flags")).toArray()) {
                flags.insert(flag.toString());
            }
            QCOMPARE(FlagProbe::scan(QString::fromUtf8(help.readAll())), flags);
        }
    }

    void cachesPerBinaryInsideTheObject() {
        FakeServerEnvironment environment;
        environment.helpTexts.insert(QStringLiteral("a.exe"), QStringLiteral("--fit --moe-hot"));
        FlagProbe probe(environment);
        const QSet<QString> expected{QStringLiteral("--fit"), QStringLiteral("--moe-hot")};
        QCOMPARE(probe.flags(QStringLiteral("a.exe"), {}), expected);
        QCOMPARE(probe.flags(QStringLiteral("a.exe"), {}), expected);
        QCOMPARE(environment.helpRuns, 1);
        QVERIFY(probe.flags(QStringLiteral("b.exe"), {}).isEmpty());
        QCOMPARE(environment.helpRuns, 2);
        FlagProbe other(environment);
        other.flags(QStringLiteral("a.exe"), {});
        QCOMPARE(environment.helpRuns, 3);
    }
};

QTEST_APPLESS_MAIN(TestFlagProbe)
#include "test_flag_probe.moc"

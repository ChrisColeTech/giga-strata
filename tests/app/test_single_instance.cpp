#include "single_instance.h"

#include <QSignalSpy>
#include <QUuid>
#include <QtTest>

// A fresh key per run, so the test never talks to a running Giga Strata.
class TestSingleInstance : public QObject {
    Q_OBJECT

private slots:
    void secondClaimRaisesTheFirst() {
        const QString key = QStringLiteral("giga-strata-test-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        SingleInstance first(key);
        QSignalSpy activationSpy(&first, &SingleInstance::activationRequested);
        QVERIFY(first.claim());

        SingleInstance second(key);
        QVERIFY(!second.claim());
        QVERIFY(activationSpy.count() > 0 || activationSpy.wait(2000));
    }

    void keyIsStablePerFolderAndCaseInsensitive() {
        const QString lower = SingleInstance::keyForInstallation(QStringLiteral("d:/projects/giga-strata/build/release/bin"));
        const QString mixed = SingleInstance::keyForInstallation(QStringLiteral("D:/Projects/giga-strata/build/release/bin/"));
        const QString other = SingleInstance::keyForInstallation(QStringLiteral("D:/Projects/other/bin"));
        QCOMPARE(lower, mixed);
        QVERIFY(lower != other);
        QVERIFY(lower.startsWith(QStringLiteral("giga-strata-")));
    }
};

QTEST_GUILESS_MAIN(TestSingleInstance)
#include "test_single_instance.moc"

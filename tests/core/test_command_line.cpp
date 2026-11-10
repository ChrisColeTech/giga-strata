#include "launch/command_line.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtTest>

// data/list2cmdline.json holds [argv, subprocess.list2cmdline(argv)] pairs from CPython 3.12:
// hand-picked backslash/quote/space edge cases plus 60 seeded random ones.
class TestCommandLine : public QObject {
    Q_OBJECT

private slots:
    void matchesPythonList2cmdline() {
        QFile file(QStringLiteral(GIGA_STRATA_TEST_DATA "/list2cmdline.json"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonArray cases = QJsonDocument::fromJson(file.readAll()).array();
        QVERIFY(cases.size() > 50);
        for (const QJsonValue &entry : cases) {
            QStringList argv;
            for (const QJsonValue &argument : entry.toArray().at(0).toArray()) {
                argv.append(argument.toString());
            }
            QCOMPARE(CommandLine::join(argv), entry.toArray().at(1).toString());
        }
    }
};

QTEST_APPLESS_MAIN(TestCommandLine)
#include "test_command_line.moc"

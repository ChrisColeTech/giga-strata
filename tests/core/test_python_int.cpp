#include "common/python_int.h"

#include <QtTest>

class TestPythonInt : public QObject {
    Q_OBJECT

private slots:
    void acceptsWhatPythonIntAccepts() {
        QCOMPARE(python_int::parse(QStringLiteral(" +1_0 ")), std::optional<qint64>(10));
        QCOMPARE(python_int::parse(QStringLiteral("-007")), std::optional<qint64>(-7));
        QCOMPARE(python_int::parse(QString::fromUtf8("\xD9\xA1\xD9\xA2")), std::optional<qint64>(12));
        QCOMPARE(python_int::parse(QStringLiteral("\x1c" "5\x1f")), std::optional<qint64>(5));
    }

    void rejectsWhatPythonIntRejects() {
        QVERIFY(!python_int::parse(QString()));
        QVERIFY(!python_int::parse(QStringLiteral("1__0")));
        QVERIFY(!python_int::parse(QStringLiteral("_1")));
        QVERIFY(!python_int::parse(QStringLiteral("1_")));
        QVERIFY(!python_int::parse(QStringLiteral("0rc1")));
        QVERIFY(!python_int::parse(QStringLiteral("+")));
        QVERIFY(!python_int::parse(QStringLiteral("99999999999999999999")));
    }
};

QTEST_APPLESS_MAIN(TestPythonInt)
#include "test_python_int.moc"

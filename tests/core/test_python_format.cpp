#include "common/python_format.h"

#include <QtTest>

// Expected strings were produced by CPython 3.12 format() on the same doubles.
class TestPythonFormat : public QObject {
    Q_OBJECT

private slots:
    void fixedRoundsExactValueTiesToEven_data() {
        QTest::addColumn<double>("value");
        QTest::addColumn<QString>("oneDigit");
        QTest::addColumn<QString>("percent");
        QTest::addColumn<QString>("noDigits");
        const auto row = [](double value, const char *oneDigit, const char *percent, const char *noDigits) {
            QTest::addRow("%g", value) << value << QString::fromLatin1(oneDigit) << QString::fromLatin1(percent)
                                        << QString::fromLatin1(noDigits);
        };
        row(0.25, "0.2", "25", "0");
        row(0.75, "0.8", "75", "1");
        row(1.25, "1.2", "125", "1");
        row(2.5, "2.5", "250", "2");
        row(0.35, "0.3", "35", "0");
        row(0.05, "0.1", "5", "0");
        row(12.45, "12.4", "1245", "12");
        row(3.25, "3.2", "325", "3");
        row(1.5e-7, "0.0", "0", "0");
        row(99.95, "100.0", "9995", "100");
        row(0.0, "0.0", "0", "0");
    }

    void fixedRoundsExactValueTiesToEven() {
        QFETCH(double, value);
        QFETCH(QString, oneDigit);
        QFETCH(QString, percent);
        QFETCH(QString, noDigits);
        QCOMPARE(python_format::fixed(value, 1), oneDigit);
        QCOMPARE(python_format::fixed(value * 100, 0), percent);
        QCOMPARE(python_format::fixed(value, 0), noDigits);
    }

    void gibAndThousandsAndLists() {
        QCOMPARE(python_format::gib(3LL * 1024 * 1024 * 1024 + 268435456), QStringLiteral("3.2 GiB"));
        QCOMPARE(python_format::thousands(1234567), QStringLiteral("1,234,567"));
        QCOMPARE(python_format::thousands(0), QStringLiteral("0"));
        QCOMPARE(python_format::thousands(20000), QStringLiteral("20,000"));
        QCOMPARE(python_format::intList({0, 1}), QStringLiteral("[0, 1]"));
        QCOMPARE(python_format::intList({}), QStringLiteral("[]"));
    }
};

QTEST_APPLESS_MAIN(TestPythonFormat)
#include "test_python_format.moc"

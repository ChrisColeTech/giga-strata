#include "common/python_json.h"

#include <QtTest>

class TestPythonJson : public QObject {
    Q_OBJECT

private slots:
    void quoteMatchesJsonDumps() {
        // Expected strings printed by json.dumps in CPython 3.12.
        QCOMPARE(python_json::quote(QStringLiteral("a\"b\\c")), QStringLiteral("\"a\\\"b\\\\c\""));
        QCOMPARE(python_json::quote(QString::fromUtf8("Mod\xC3\xA8le \xE6\xA8\xA1")),
                 QStringLiteral("\"Mod\\u00e8le \\u6a21\""));
        QCOMPARE(python_json::quote(QString::fromUtf8("\xF0\x9F\x98\x80")), QStringLiteral("\"\\ud83d\\ude00\""));
        QCOMPARE(python_json::quote(QStringLiteral("\t\x01\x7f")), QStringLiteral("\"\\t\\u0001\\u007f\""));
    }

    void parseRejectsWhatJsonLoadRejects() {
        QVERIFY(python_json::parse("{\"a\": 1}"));
        QVERIFY(!python_json::parse("\xEF\xBB\xBF{}"));
        QVERIFY(!python_json::parse("{\"a\": \"\xFF\"}"));
        QVERIFY(!python_json::parse("{\"a\": "));
    }
};

QTEST_APPLESS_MAIN(TestPythonJson)
#include "test_python_json.moc"

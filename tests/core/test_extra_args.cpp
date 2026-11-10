#include "launch/extra_args.h"

#include "golden_cases.h"

#include <QtTest>

// The golden cases are the README section 8 item 5 quirks; 500 random strings are also compared
// with CPython by the out-of-repo harness through tests/tools/launch_cli.
class TestExtraArgs : public QObject {
    Q_OBJECT

private slots:
    void tokenizesLikeReadSettings() {
        const auto cases = GoldenCases::small(QStringLiteral("shlex"));
        QVERIFY(cases.size() >= 36);
        for (const auto &[input, expected] : cases) {
            const auto tokens = ExtraArgs::split(input.value(QStringLiteral("extra")).toString());
            const QJsonValue expectedTokens = expected.value(QStringLiteral("tokens"));
            QVERIFY2(tokens.ok() == !expectedTokens.isNull(), qPrintable(input.value(QStringLiteral("id")).toString()));
            if (!tokens.ok()) {
                continue;
            }
            QStringList expectedList;
            for (const QJsonValue &token : expectedTokens.toArray()) {
                expectedList.append(token.toString());
            }
            QCOMPARE(tokens.value(), expectedList);
        }
    }
};

QTEST_APPLESS_MAIN(TestExtraArgs)
#include "test_extra_args.moc"

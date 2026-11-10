#include "moe/hot_file.h"

#include "golden_cases.h"

#include <QtTest>

namespace {

HotMap hotFrom(const QJsonObject &json) {
    HotMap hot;
    for (auto layer = json.begin(); layer != json.end(); ++layer) {
        QVector<int> experts;
        for (const QJsonValue &expert : layer.value().toArray()) {
            experts.append(expert.toInt());
        }
        hot.insert(layer.key().toInt(), experts);
    }
    return hot;
}

} // namespace

class TestHotFile : public QObject {
    Q_OBJECT

private slots:
    void readsAndWritesLikeTheReference() {
        const auto cases = GoldenCases::small(QStringLiteral("hot_file"));
        QVERIFY(cases.size() >= 12);
        for (const auto &[input, expected] : cases) {
            const QString id = input.value(QStringLiteral("id")).toString();
            if (input.contains(QStringLiteral("write"))) {
                const QJsonObject write = input.value(QStringLiteral("write")).toObject();
                const QByteArray text = HotFile::write(write.value(QStringLiteral("n_expert")).toInt(),
                                                       hotFrom(write.value(QStringLiteral("hot")).toObject()));
                QCOMPARE(QString::fromLatin1(text), expected.value(QStringLiteral("text")).toString());
                continue;
            }
            const QString path = input.value(QStringLiteral("path")).toString();
            const auto hot = HotFile::read(input.value(QStringLiteral("text")).toString().toLatin1(),
                                           input.value(QStringLiteral("n_expert")).toInt(), path);
            const QJsonValue error = expected.value(QStringLiteral("error"));
            QVERIFY2(hot.ok() == error.isNull(), qPrintable(id));
            if (hot.ok()) {
                QCOMPARE(hot.value(), hotFrom(expected.value(QStringLiteral("hot")).toObject()));
            } else if (error.toObject().value(QStringLiteral("message")).toString().startsWith(path)) {
                QCOMPARE(hot.error(), error.toObject().value(QStringLiteral("message")).toString());
            }
        }
    }

    void rejectsNonAsciiAndPythonIntQuirksAreKept() {
        QVERIFY(!HotFile::read("llama-moe-hot 1 n_expert=4\n0: \xC3\xA9\n", 4).ok());
        const auto hot = HotFile::read("llama-moe-hot 1 n_expert=4\n +1_0 : 3 0\x0b" "2: none\n", 4);
        QVERIFY(hot.ok());
        QCOMPARE(hot.value().value(10), QVector<int>({0, 3}));
        QCOMPARE(hot.value().value(2), QVector<int>());
    }
};

QTEST_APPLESS_MAIN(TestHotFile)
#include "test_hot_file.moc"

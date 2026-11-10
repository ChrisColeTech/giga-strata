#include "moe/usage_io.h"

#include "golden_cases.h"
#include "moe/hot_cost_model.h"
#include "moe/usage_source.h"

#include <QCryptographicHash>
#include <QFile>
#include <QMap>
#include <QtTest>

namespace {

// UsageSource over fixture bytes, keyed by the relative path the reference read them from.
class FixtureUsageSource : public UsageSource {
public:
    void add(const QString &path, const QByteArray &bytes) { m_files.insert(path, bytes); }
    bool exists(const QString &path) const override { return m_files.contains(path); }
    std::optional<QByteArray> readAll(const QString &path) const override {
        return m_files.contains(path) ? std::optional<QByteArray>(m_files.value(path)) : std::nullopt;
    }
    std::optional<qint64> fileSize(const QString &path) const override {
        return m_files.contains(path) ? std::optional<qint64>(m_files.value(path).size()) : std::nullopt;
    }
    std::optional<qint64> modifiedTimeNs(const QString &) const override { return std::nullopt; }

private:
    QMap<QString, QByteArray> m_files;
};

// Python repr of a count: the profiles hold whole numbers, which repr prints as "123.0".
QString pythonRepr(double value) {
    return QString::number(qint64(value)) + QStringLiteral(".0");
}

// The digest README section 6 describes: sha1 of json.dumps({str(layer): [floats]}, (",", ":")).
QString countsDigest(const UsageCounts &counts) {
    QStringList layers;
    for (auto layer = counts.cbegin(); layer != counts.cend(); ++layer) {
        QStringList values;
        for (const double value : layer.value()) {
            values.append(pythonRepr(value));
        }
        layers.append(QStringLiteral("\"%1\":[%2]").arg(layer.key()).arg(values.join(u',')));
    }
    const QByteArray text = (u'{' + layers.join(u',') + u'}').toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(text, QCryptographicHash::Sha1).toHex());
}

void compareCounts(const Result<UsageCounts> &actual, const QJsonObject &expected, const QString &label) {
    const bool shouldFail = !expected.value(QStringLiteral("error")).isNull();
    QVERIFY2(actual.ok() != shouldFail, qPrintable(label));
    if (shouldFail) {
        return;
    }
    const QJsonObject value = expected.value(QStringLiteral("value")).toObject();
    QJsonArray layers;
    for (const int layer : actual.value().keys()) {
        layers.append(layer);
    }
    QCOMPARE(layers, value.value(QStringLiteral("layers")).toArray());
    QCOMPARE(countsDigest(actual.value()), value.value(QStringLiteral("sha1")).toString());
    const QJsonObject sums = value.value(QStringLiteral("sums")).toObject();
    for (auto layer = actual.value().cbegin(); layer != actual.value().cend(); ++layer) {
        QCOMPARE(HotCostModel::pythonSum(layer.value()), sums.value(QString::number(layer.key())).toDouble());
    }
}

} // namespace

class TestUsageIo : public QObject {
    Q_OBJECT

private slots:
    void matchesTheReferenceOnEveryProfile() {
        const auto cases = GoldenCases::small(QStringLiteral("usage_io"));
        QVERIFY(cases.size() >= 15);
        for (const auto &[input, expected] : cases) {
            const QString path = input.value(QStringLiteral("path")).toString();
            QFile file(GoldenCases::root() + QStringLiteral("/fixtures/usage/") +
                       input.value(QStringLiteral("profile")).toString() + QStringLiteral(".usage.json"));
            QVERIFY(file.open(QIODevice::ReadOnly));
            const QByteArray bytes = file.readAll();
            FixtureUsageSource source;
            source.add(path, bytes);

            const auto tokens = UsageIo::usageTokens(source, {path});
            const QJsonObject expectedTokens = expected.value(QStringLiteral("usage_tokens")).toObject();
            QCOMPARE(tokens.ok(), expectedTokens.value(QStringLiteral("error")).isNull());
            if (tokens.ok()) {
                const QJsonArray pair = expectedTokens.value(QStringLiteral("value")).toArray();
                QCOMPARE(tokens.value().first, pair.at(0).toInteger());
                QCOMPARE(tokens.value().second, pair.at(1).toInteger());
            }
            compareCounts(UsageIo::readUsage(bytes), expected.value(QStringLiteral("read_usage")).toObject(), path);
            compareCounts(UsageIo::readUsage(bytes, QStringLiteral("decode")),
                          expected.value(QStringLiteral("read_usage_decode")).toObject(), path);
            compareCounts(UsageIo::readUsage(bytes, QStringLiteral("batch")),
                          expected.value(QStringLiteral("read_usage_batch")).toObject(), path);
            const auto counts = UsageIo::readCounts(source, {path});
            compareCounts(counts, expected.value(QStringLiteral("read_counts")).toObject(), path);
            const QJsonObject countsError = expected.value(QStringLiteral("read_counts"))
                                                .toObject()
                                                .value(QStringLiteral("error"))
                                                .toObject();
            if (countsError.value(QStringLiteral("type")).toString() == QStringLiteral("ValueError")) {
                QCOMPARE(counts.error(), countsError.value(QStringLiteral("message")).toString());
            }
        }
    }

    void sumsFilesLayerByLayerAndKeepsTheShorterLength() {
        FixtureUsageSource source;
        source.add(QStringLiteral("a.json"),
                   R"({"format":"llama-moe-usage","tokens":{"decode":1},"counts":{"decode":[[1,2,3],[],[4]]}})");
        source.add(QStringLiteral("b.JSON"),
                   R"({"format":"llama-moe-usage","tokens":{"decode":1},"counts":{"decode":[[10,20],null,[5]]}})");
        const auto counts = UsageIo::readCounts(source, {QStringLiteral("a.json"), QStringLiteral("b.JSON")});
        QVERIFY(counts.ok());
        QCOMPARE(counts.value().value(0), QVector<double>({11.0, 22.0}));
        QVERIFY(!counts.value().contains(1));
        QCOMPARE(counts.value().value(2), QVector<double>({9.0}));
    }

    void rejectsImatrixAndUnreadableFiles() {
        FixtureUsageSource source;
        QVERIFY(!UsageIo::readCounts(source, {QStringLiteral("imatrix.gguf")}).ok());
        QVERIFY(!UsageIo::readCounts(source, {QStringLiteral("missing.json")}).ok());
        QVERIFY(UsageIo::usageTokens(source, {QStringLiteral("imatrix.gguf")}).ok());
        QVERIFY(!UsageIo::readUsage("\xEF\xBB\xBF{\"format\":\"llama-moe-usage\"}").ok());
    }

    void pythonSumIsCompensated() {
        // CPython 3.12: sum([1e16, 1.0, -1e16]) is 1.0, a plain running total gives 0.0.
        QCOMPARE(HotCostModel::pythonSum({1e16, 1.0, -1e16}), 1.0);
        QCOMPARE(HotCostModel::pythonSum({0.1, 0.2, 0.3}), 0.6);
    }
};

QTEST_APPLESS_MAIN(TestUsageIo)
#include "test_usage_io.moc"

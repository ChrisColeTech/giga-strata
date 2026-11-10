#include "moe/hot_cache.h"

#include "golden_cases.h"

#include <QDir>
#include <QSet>
#include <QTemporaryDir>
#include <QtTest>

namespace {

FileStamp stampFrom(const QJsonValue &value) {
    const QJsonArray array = value.toArray();
    return {array.at(0).toString(), array.at(1).toInteger(), array.at(2).toInteger()};
}

HotCacheKey keyFrom(const QJsonObject &input) {
    HotCacheKey key;
    key.model = stampFrom(input.value(QStringLiteral("model")));
    for (const QJsonValue &file : input.value(QStringLiteral("files")).toArray()) {
        key.usageFiles.append(stampFrom(file));
    }
    for (const QJsonValue &id : input.value(QStringLiteral("gpu_ids")).toArray()) {
        key.gpuIds.append(id.toInt());
    }
    key.contextSize = input.value(QStringLiteral("ctx")).toInteger();
    key.marginMib = input.value(QStringLiteral("margin")).toInteger();
    if (input.value(QStringLiteral("ubatch")).isDouble()) {
        key.microBatch = input.value(QStringLiteral("ubatch")).toInt();
    }
    key.reserveBytes = input.value(QStringLiteral("reserve")).toInteger();
    return key;
}

QStringList strings(const QJsonValue &value) {
    QStringList out;
    for (const QJsonValue &item : value.toArray()) {
        out.append(item.toString());
    }
    return out;
}

} // namespace

class TestHotCache : public QObject {
    Q_OBJECT

private slots:
    void namesAndPrunesLikeHotCachePath() {
        const QString hotDirectory = QStringLiteral("moe_usage\\hot");
        const auto cases = GoldenCases::small(QStringLiteral("hot_cache_key"));
        QVERIFY(!cases.isEmpty());
        for (const auto &[input, expected] : cases) {
            const HotCacheKey key = keyFrom(input);
            QCOMPARE(HotCache::keyJson(key), expected.value(QStringLiteral("key_json")).toString());
            QCOMPARE(HotCache::fileName(key), expected.value(QStringLiteral("name")).toString());
            const QString path = HotCache::filePath(hotDirectory, key);
            QCOMPARE(path, expected.value(QStringLiteral("path")).toString());

            const QStringList before = strings(input.value(QStringLiteral("hot_dir_before")));
            QSet<QString> after(before.cbegin(), before.cend());
            for (const QString &stale : HotCache::staleFiles(hotDirectory, before, key.model.path, path)) {
                QVERIFY(stale.startsWith(hotDirectory + u'\\'));
                after.remove(stale.mid(hotDirectory.size() + 1));
            }
            QStringList sortedAfter(after.cbegin(), after.cend());
            sortedAfter.sort();
            QCOMPARE(sortedAfter, strings(expected.value(QStringLiteral("hot_dir_after"))));
        }
    }

    void writesTheFileAndRemovesOnlyStaleOnes() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString hotDirectory = QDir::toNativeSeparators(root.filePath(QStringLiteral("hot")));
        QVERIFY(QDir().mkpath(hotDirectory));
        const QString kept = QStringLiteral("m-ABCDEF012345.moe-hot.txt");
        const QString other = QStringLiteral("other-0123456789ab.moe-hot.txt");
        const QStringList names{QStringLiteral("m-0123456789ab.moe-hot.txt"), kept, other};
        for (const QString &name : names) {
            QFile file(hotDirectory + u'\\' + name);
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        const QString current = hotDirectory + QStringLiteral("\\m-ffffffffffff.moe-hot.txt");
        QVERIFY(HotCache::writeAndPrune(hotDirectory, QStringLiteral("N:\\x\\m.gguf"), current, "text\n").ok());
        QStringList left = QDir(hotDirectory).entryList(QDir::Files);
        left.sort();
        QCOMPARE(left, QStringList({kept, QStringLiteral("m-ffffffffffff.moe-hot.txt"), other}));
    }
};

QTEST_APPLESS_MAIN(TestHotCache)
#include "test_hot_cache.moc"

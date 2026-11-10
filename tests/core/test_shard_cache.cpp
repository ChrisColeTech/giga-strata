#include "gguf/gguf_reader.h"
#include "gguf/shard_cache.h"

#include "gguf_builder.h"
#include "memory_model_source.h"

#include <QtTest>

namespace {

// MemoryModelSource with a ShardCache attached.
class CachedSource : public MemoryModelSource {
public:
    ShardCache cache{2};
    ShardCache *shardCache() const override { return const_cast<ShardCache *>(&cache); }
};

} // namespace

class TestShardCache : public QObject {
    Q_OBJECT

private slots:
    void secondReadComesFromTheCache() {
        CachedSource source;
        source.addFile(QStringLiteral("a.gguf"), GgufBuilder().addTensor(QStringLiteral("t"), 0).header(), 100, 1);
        QVERIFY(GgufReader::read(source, QStringLiteral("a.gguf")).ok());
        const int reads = source.prefixReadCount();
        const auto again = GgufReader::read(source, QStringLiteral("a.gguf"));
        QVERIFY(again.ok());
        QCOMPARE(source.prefixReadCount(), reads);
        QVERIFY(again.value().tensorSizes.contains(QStringLiteral("t")));
    }

    void anEditedFileIsReadAgain() {
        CachedSource source;
        const QByteArray header = GgufBuilder().addTensor(QStringLiteral("t"), 0).header();
        source.addFile(QStringLiteral("a.gguf"), header, 100, 1);
        QVERIFY(GgufReader::read(source, QStringLiteral("a.gguf")).ok());
        source.addFile(QStringLiteral("a.gguf"), header, 100, 2);
        const int reads = source.prefixReadCount();
        QVERIFY(GgufReader::read(source, QStringLiteral("a.gguf")).ok());
        QVERIFY(source.prefixReadCount() > reads);
    }

    void oldestEntryLeavesFirst() {
        ShardCache cache(2);
        cache.insert(QStringLiteral("a"), GgufShard());
        cache.insert(QStringLiteral("b"), GgufShard());
        cache.insert(QStringLiteral("c"), GgufShard());
        QVERIFY(!cache.find(QStringLiteral("a")));
        QVERIFY(cache.find(QStringLiteral("b")));
        QVERIFY(cache.find(QStringLiteral("c")));
    }
};

QTEST_GUILESS_MAIN(TestShardCache)
#include "test_shard_cache.moc"

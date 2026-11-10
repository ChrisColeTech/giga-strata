#include "gguf/shard_finder.h"
#include "memory_model_source.h"

#include <QtTest>

class TestShardFinder : public QObject {
    Q_OBJECT

private slots:
    void singleFileIsItsOwnShard() {
        MemoryModelSource source;
        QCOMPARE(ShardFinder::shardPaths(source, QStringLiteral("D:\\m\\model.gguf")).value(), QStringList{QStringLiteral("D:\\m\\model.gguf")});
    }

    void splitSetExpandsWithLowercaseExtension() {
        MemoryModelSource source;
        source.addFile(QStringLiteral("D:\\m\\M-00001-of-00002.gguf"), {});
        source.addFile(QStringLiteral("D:\\m\\M-00002-of-00002.gguf"), {});
        const auto paths = ShardFinder::shardPaths(source, QStringLiteral("D:\\m\\M-00002-of-00002.GGUF"));
        QCOMPARE(paths.value(), QStringList({QStringLiteral("D:\\m\\M-00001-of-00002.gguf"), QStringLiteral("D:\\m\\M-00002-of-00002.gguf")}));
    }

    void missingShardsAreNamed() {
        MemoryModelSource source;
        source.addFile(QStringLiteral("D:\\m\\M-00001-of-00003.gguf"), {});
        const auto paths = ShardFinder::shardPaths(source, QStringLiteral("D:\\m\\M-00001-of-00003.gguf"));
        QVERIFY(!paths.ok());
        QCOMPARE(paths.error(), QStringLiteral("missing split file(s): M-00002-of-00003.gguf, M-00003-of-00003.gguf"));
    }
};

QTEST_APPLESS_MAIN(TestShardFinder)
#include "test_shard_finder.moc"

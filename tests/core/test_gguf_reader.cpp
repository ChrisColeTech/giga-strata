#include "gguf/gguf_reader.h"
#include "gguf_builder.h"
#include "memory_model_source.h"

#include <QtTest>

class TestGgufReader : public QObject {
    Q_OBJECT

private slots:
    void sizesComeFromOffsetGapsAndFileEnd() {
        const QByteArray header = GgufBuilder().addTensor(QStringLiteral("b"), 100).addTensor(QStringLiteral("a"), 0).header();
        MemoryModelSource source;
        source.addFile(QStringLiteral("m.gguf"), header, header.size() + 250);
        const auto shard = GgufReader::read(source, QStringLiteral("m.gguf"));
        QVERIFY2(shard.ok(), qPrintable(shard.error()));
        QCOMPARE(shard.value().tensorSizes.value(QStringLiteral("a")), 100);
        QCOMPARE(shard.value().tensorSizes.value(QStringLiteral("b")), 150);
    }

    void customAlignmentMovesDataStart() {
        GgufBuilder builder;
        builder.addUint32(QStringLiteral("general.alignment"), 64).addTensor(QStringLiteral("t"), 0);
        const QByteArray header = builder.header(64);
        MemoryModelSource source;
        source.addFile(QStringLiteral("m.gguf"), header, header.size() + 10);
        QCOMPARE(GgufReader::read(source, QStringLiteral("m.gguf")).value().tensorSizes.value(QStringLiteral("t")), 10);
    }

    void largeStringArraysAreSkippedExceptTokens() {
        QStringList many;
        for (int i = 0; i < 65; ++i) {
            many.append(QStringLiteral("s%1").arg(i));
        }
        GgufBuilder builder;
        builder.addStringArray(QStringLiteral("tokenizer.ggml.merges"), many)
            .addStringArray(QStringLiteral("tokenizer.ggml.tokens"), many)
            .addStringArray(QStringLiteral("small"), {QStringLiteral("x"), QStringLiteral("y")});
        MemoryModelSource source;
        source.addFile(QStringLiteral("m.gguf"), builder.header());
        const auto shard = GgufReader::read(source, QStringLiteral("m.gguf")).value();
        QVERIFY(shard.metadata.contains(QStringLiteral("tokenizer.ggml.merges")));
        QVERIFY(shard.metadata.value(QStringLiteral("tokenizer.ggml.merges")).isNull());
        QCOMPARE(shard.metadata.value(QStringLiteral("tokenizer.ggml.tokens")).toList().size(), 65);
        QCOMPARE(shard.metadata.value(QStringLiteral("small")).toStringList(), QStringList({QStringLiteral("x"), QStringLiteral("y")}));
    }

    void boolArraysDecodePerElement() {
        const QByteArray body = GgufBuilder::scalarBytes(quint32(7)) + GgufBuilder::scalarBytes(quint64(2)) + QByteArray("\x01\x00", 2);
        GgufBuilder builder;
        builder.addRawValue(QStringLiteral("flags"), 9, body);
        MemoryModelSource source;
        source.addFile(QStringLiteral("m.gguf"), builder.header());
        const QVariantList flags = GgufReader::read(source, QStringLiteral("m.gguf")).value().metadata.value(QStringLiteral("flags")).toList();
        QCOMPARE(flags, QVariantList({true, false}));
    }

    void rejectsBadMagicAndOldVersion() {
        MemoryModelSource source;
        source.addFile(QStringLiteral("x\\bad.gguf"), QByteArrayLiteral("NOPE0000"));
        QCOMPARE(GgufReader::read(source, QStringLiteral("x\\bad.gguf")).error(), QStringLiteral("bad.gguf is not a GGUF file"));
        source.addFile(QStringLiteral("old.gguf"), GgufBuilder(1).header());
        QCOMPARE(GgufReader::read(source, QStringLiteral("old.gguf")).error(), QStringLiteral("GGUF version 1 is too old"));
    }

    void rejectsUnknownValueType() {
        GgufBuilder builder;
        builder.addRawValue(QStringLiteral("weird"), 42, QByteArray());
        MemoryModelSource source;
        source.addFile(QStringLiteral("m.gguf"), builder.header());
        QCOMPARE(GgufReader::read(source, QStringLiteral("m.gguf")).error(), QStringLiteral("unknown GGUF value type 42"));
    }

    void headerLongerThanFirstReadIsRegrown() {
        QStringList tokens;
        for (int i = 0; i < 400000; ++i) {
            tokens.append(QStringLiteral("token-number-%1").arg(i));
        }
        GgufBuilder builder;
        builder.addStringArray(QStringLiteral("tokenizer.ggml.tokens"), tokens).addTensor(QStringLiteral("t"), 0);
        const QByteArray header = builder.header();
        QVERIFY(header.size() > 4 * 1024 * 1024);
        MemoryModelSource source;
        source.addFile(QStringLiteral("big.gguf"), header, header.size() + 7);
        const auto shard = GgufReader::read(source, QStringLiteral("big.gguf"));
        QVERIFY2(shard.ok(), qPrintable(shard.error()));
        QCOMPARE(shard.value().tensorSizes.value(QStringLiteral("t")), 7);
        QCOMPARE(source.prefixReadCount(), 2);
    }
};

QTEST_APPLESS_MAIN(TestGgufReader)
#include "test_gguf_reader.moc"

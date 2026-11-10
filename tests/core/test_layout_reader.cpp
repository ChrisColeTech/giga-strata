#include "gguf/layout_reader.h"

#include <QtTest>

// Edge cases of the fold in strata_planner.read_layout; real models are covered by goldens.
class TestLayoutReader : public QObject {
    Q_OBJECT

private slots:
    void expertsDenseOutputAndGlobalAreSeparated() {
        const QVariantMap meta{{QStringLiteral("general.architecture"), QStringLiteral("a")},
                               {QStringLiteral("a.block_count"), 2},
                               {QStringLiteral("a.attention.head_count"), 4},
                               {QStringLiteral("a.embedding_length"), 64}};
        const QMap<QString, qint64> sizes{{QStringLiteral("blk.0.attn_q.weight"), 10},
                                          {QStringLiteral("blk.0.ffn_up_exps.weight"), 100},
                                          {QStringLiteral("blk.1.ffn_gate_up_exps.weight"), 200},
                                          {QStringLiteral("blk.1.ffn_norm.weight"), 3},
                                          {QStringLiteral("output.weight"), 50},
                                          {QStringLiteral("output_norm.weight"), 1},
                                          {QStringLiteral("token_embd.weight"), 70}};
        const ModelLayout layout = LayoutReader::fromShards(meta, sizes);
        QCOMPARE(layout.denseBytes, QVector<qint64>({10, 3}));
        QCOMPARE(layout.expertBytes, QVector<qint64>({100, 200}));
        QCOMPARE(layout.outputBytes, 51);
        QCOMPARE(layout.cpuGlobalBytes, 70);
        QCOMPARE(layout.totalBytes, 434);
        QCOMPARE(layout.kvBytesPerToken, QVector<qint64>({4 * 32 * 2, 4 * 32 * 2}));
    }

    void tiedEmbeddingsCountTowardOutput() {
        const QVariantMap meta{{QStringLiteral("general.architecture"), QStringLiteral("a")}, {QStringLiteral("a.block_count"), 1}};
        const QMap<QString, qint64> sizes{{QStringLiteral("token_embd.weight"), 70}};
        const ModelLayout layout = LayoutReader::fromShards(meta, sizes);
        QCOMPARE(layout.outputBytes, 70);
        QCOMPARE(layout.cpuGlobalBytes, 70);
    }

    void mtpLayersAreSkippedButCountedInTotal() {
        const QVariantMap meta{{QStringLiteral("general.architecture"), QStringLiteral("a")},
                               {QStringLiteral("a.block_count"), 3},
                               {QStringLiteral("a.nextn_predict_layers"), 1}};
        const QMap<QString, qint64> sizes{{QStringLiteral("blk.2.ffn_up_exps.weight"), 500}, {QStringLiteral("output.weight"), 1}};
        const ModelLayout layout = LayoutReader::fromShards(meta, sizes);
        QCOMPARE(layout.layerCount, 2);
        QCOMPARE(layout.skippedLayerCount, 1);
        QCOMPARE(layout.expertBytes, QVector<qint64>({0, 0}));
        QCOMPARE(layout.totalBytes, 501);
    }

    void kvVariants() {
        QVariantMap meta{{QStringLiteral("general.architecture"), QStringLiteral("a")},
                         {QStringLiteral("a.block_count"), 4},
                         {QStringLiteral("a.attention.head_count"), QVariantList{8, 16}},
                         {QStringLiteral("a.attention.head_count_kv"), QVariantList{2, 0, 2, 2}},
                         {QStringLiteral("a.attention.key_length"), 10},
                         {QStringLiteral("a.full_attention_interval"), 2}};
        QCOMPARE(LayoutReader::fromShards(meta, {}).kvBytesPerToken, QVector<qint64>({0, 0, 0, 2 * 20 * 2}));
        meta.insert(QStringLiteral("a.attention.kv_lora_rank"), 512);
        meta.remove(QStringLiteral("a.full_attention_interval"));
        QCOMPARE(LayoutReader::fromShards(meta, {}).kvBytesPerToken, QVector<qint64>({576 * 2, 0, 576 * 2, 576 * 2}));
    }

    void glmStopTokensAreFixed() {
        const QVariantMap meta{{QStringLiteral("general.architecture"), QStringLiteral("a")},
                               {QStringLiteral("tokenizer.ggml.pre"), QStringLiteral("glm4")},
                               {QStringLiteral("tokenizer.ggml.eos_token_id"), 0},
                               {QStringLiteral("tokenizer.ggml.tokens"),
                                QVariantList{QStringLiteral("<|endoftext|>"), QStringLiteral("<|user|>"), QStringLiteral("<|observation|>")}}};
        const ModelLayout layout = LayoutReader::fromShards(meta, {});
        QCOMPARE(layout.stopTokenOverrides, QStringList({QStringLiteral("tokenizer.ggml.eos_token_id=int:1"),
                                                         QStringLiteral("tokenizer.ggml.eot_token_id=int:2")}));
        QCOMPARE(layout.vocabSize, 3);
    }
};

QTEST_APPLESS_MAIN(TestLayoutReader)
#include "test_layout_reader.moc"

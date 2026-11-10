#include "gguf/quant_name.h"

#include <QtTest>

class TestQuantName : public QObject {
    Q_OBJECT

private slots:
    void fileTypeWins() {
        QCOMPARE(QuantName::of(15, QStringLiteral("x-IQ3_S.gguf")), QStringLiteral("Q4_K_M"));
        QCOMPARE(QuantName::fromFileType(32), QStringLiteral("BF16"));
        QCOMPARE(QuantName::fromFileType(4), QString());
    }

    void fileNameIsTheFallback() {
        QCOMPARE(QuantName::of(QVariant(), QStringLiteral("Qwen3.8-Flash-Next-GSQ-RCO-abliterated-IQ3_S-00001-of-00002.gguf")),
                 QStringLiteral("IQ3_S"));
        QCOMPARE(QuantName::of(9999, QStringLiteral("GLM-5.3-Flash-GSQ-RCO-3.0bit-q4kattn.gguf")), QStringLiteral("3.0bit"));
        QCOMPARE(QuantName::fromFileName(QStringLiteral("model-UD-IQ3_XXS.gguf")), QStringLiteral("UD-IQ3_XXS"));
        QCOMPARE(QuantName::fromFileName(QStringLiteral("Qwen3-8B-Q4_K_M.gguf")), QStringLiteral("Q4_K_M"));
        QCOMPARE(QuantName::fromFileName(QStringLiteral("plain.gguf")), QString());
    }
};

QTEST_GUILESS_MAIN(TestQuantName)
#include "test_quant_name.moc"

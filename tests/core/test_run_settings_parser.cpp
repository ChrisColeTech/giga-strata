#include "launch/run_settings_parser.h"

#include <QtTest>

// read_settings edge cases; every real combination also runs through the golden scenarios.
class TestRunSettingsParser : public QObject {
    Q_OBJECT

private:
    static QVariantMap valid() {
        return {{QStringLiteral("model"), QStringLiteral(" N:\\m.gguf ")},
                {QStringLiteral("gpus"), QStringLiteral("0, 1,")},
                {QStringLiteral("ctx"), QStringLiteral(" 32_768 ")},
                {QStringLiteral("margin"), QStringLiteral("1536")},
                {QStringLiteral("ubatch"), QStringLiteral(" AUTO ")},
                {QStringLiteral("extra"), QStringLiteral("-a \"my model\"")},
                {QStringLiteral("mmproj"), QStringLiteral(" \"N:\\p.gguf\" ")},
                {QStringLiteral("mmap"), false}};
    }

private slots:
    void convertsLikePython() {
        const auto settings = RunSettingsParser::parse(valid());
        QVERIFY(settings.ok());
        QCOMPARE(settings.value().modelPath, QStringLiteral("N:\\m.gguf"));
        QCOMPARE(settings.value().gpuIds, QVector<int>({0, 1}));
        QCOMPARE(settings.value().contextSize, 32768);
        QVERIFY(!settings.value().microBatch);
        QCOMPARE(settings.value().extraArgs, QStringList({QStringLiteral("-a"), QStringLiteral("my model")}));
        QCOMPARE(settings.value().mmprojPath, QStringLiteral("N:\\p.gguf"));
        QVERIFY(!settings.value().useMmap);
        QVERIFY(settings.value().useFlashAttention);
    }

    void refusesWhatPythonRefuses() {
        QVariantMap values = valid();
        values.insert(QStringLiteral("model"), QString());
        QCOMPARE(RunSettingsParser::parse(values).error(), RunSettingsParser::missingModelMessage());
        for (const auto &[key, value] : {std::pair{"ctx", "32k"}, {"gpus", "0;1"}, {"ubatch", "big"}, {"extra", "\"open"}}) {
            values = valid();
            values.insert(QLatin1String(key), QLatin1String(value));
            QCOMPARE(RunSettingsParser::parse(values).error(), RunSettingsParser::notWholeNumbersMessage());
        }
        values = valid();
        values.insert(QStringLiteral("ubatch"), QStringLiteral("0512"));
        QCOMPARE(*RunSettingsParser::parse(values).value().microBatch, 512);
    }
};

QTEST_GUILESS_MAIN(TestRunSettingsParser)
#include "test_run_settings_parser.moc"

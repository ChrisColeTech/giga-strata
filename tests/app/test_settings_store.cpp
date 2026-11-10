#include "settings_file.h"
#include "settings_store.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace {

QJsonObject readJson(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

void writeBytes(const QString &filePath, const QByteArray &bytes) {
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(bytes);
}

} // namespace

// Every test works on a copy in a temporary folder: the real launcher_settings.json is never written.
class TestSettingsStore : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_folder;
    QString m_filePath;

private slots:
    void init() {
        QVERIFY(m_folder.isValid());
        m_filePath = m_folder.filePath(QStringLiteral("launcher_settings.json"));
        QFile::remove(m_filePath);
    }

    void readsRealPythonFile() {
        QVERIFY(QFile::copy(QStringLiteral(GIGA_STRATA_APP_FIXTURES "/real_launcher_settings.json"), m_filePath));
        QFile::setPermissions(m_filePath, QFile::ReadOwner | QFile::WriteOwner);
        const SettingsStore store(m_filePath);
        QVERIFY(store.loadError().isEmpty());
        QCOMPARE(store.contextSize(), QStringLiteral("200000"));
        QCOMPARE(store.gpus(), QStringLiteral("0,1"));
        QCOMPARE(store.port(), QStringLiteral("1234"));
        QCOMPARE(store.ubatch(), QStringLiteral("auto"));
        QVERIFY(store.useMmap());
        QVERIFY(store.autoServer());
        QVERIFY(!store.usePerExpert());
        QVERIFY(store.model().endsWith(QStringLiteral("GLM-5.3-Flash-GSQ-RCO-3.0bit-q4kattn.gguf")));
    }

    void rewriteKeepsPythonKeysAndTypes() {
        const QString fixture = QStringLiteral(GIGA_STRATA_APP_FIXTURES "/real_launcher_settings.json");
        QVERIFY(QFile::copy(fixture, m_filePath));
        QFile::setPermissions(m_filePath, QFile::ReadOwner | QFile::WriteOwner);
        {
            SettingsStore store(m_filePath);
            store.setContextSize(QStringLiteral("65536"));
            store.setUseMtp(false);
        }
        const QJsonObject original = readJson(fixture);
        const QJsonObject rewritten = readJson(m_filePath);
        for (auto it = original.begin(); it != original.end(); ++it) {
            QVERIFY2(rewritten.contains(it.key()), qPrintable(it.key()));
            QCOMPARE(rewritten.value(it.key()).type(), it.value().type());
        }
        QCOMPARE(rewritten.value(QStringLiteral("ctx")), QJsonValue(QStringLiteral("65536")));
        QCOMPARE(rewritten.value(QStringLiteral("mtp")), QJsonValue(false));
        QCOMPARE(rewritten.value(QStringLiteral("server_cache")), original.value(QStringLiteral("server_cache")));
        QCOMPARE(rewritten.value(QStringLiteral("server_candidates")), original.value(QStringLiteral("server_candidates")));
        QCOMPARE(rewritten.value(QStringLiteral("binary")), original.value(QStringLiteral("binary")));
    }

    void preservesUnknownKeysAndAppendsNewOnes() {
        writeBytes(m_filePath, R"({"ctx": "8192", "future_key": {"nested": [1, 2]}, "flag_x": true})");
        {
            SettingsStore store(m_filePath);
            store.setUiSplitRatio(0.5);
        }
        const QJsonObject rewritten = readJson(m_filePath);
        QCOMPARE(rewritten.value(QStringLiteral("future_key")).toObject().value(QStringLiteral("nested")).toArray().size(), 2);
        QCOMPARE(rewritten.value(QStringLiteral("flag_x")), QJsonValue(true));
        QCOMPARE(rewritten.value(QStringLiteral("ctx")), QJsonValue(QStringLiteral("8192")));
        QCOMPARE(rewritten.value(QStringLiteral("uiSplitRatio")).toDouble(), 0.5);
        const QByteArray text = SettingsFile::serialize(rewritten, {QStringLiteral("ctx")});
        QVERIFY(text.indexOf("\"ctx\"") < text.indexOf("\"flag_x\""));
    }

    void missingFileUsesDefaultsAndWritesNothingUntilChanged() {
        {
            const SettingsStore store(m_filePath);
            QVERIFY(store.loadError().isEmpty());
            QCOMPARE(store.contextSize(), QStringLiteral("32768"));
            QCOMPARE(store.port(), QStringLiteral("8080"));
            QVERIFY(store.useFlashAttention());
        }
        QVERIFY(!QFile::exists(m_filePath));
    }

    void corruptFileFallsBackToDefaults() {
        writeBytes(m_filePath, "{ not json");
        const SettingsStore store(m_filePath);
        QVERIFY(!store.loadError().isEmpty());
        QCOMPARE(store.margin(), QStringLiteral("1536"));
    }

    void numbersFromHandEditsBecomeStrings() {
        writeBytes(m_filePath, R"({"ctx": 131072, "mmap": "yes"})");
        const SettingsStore store(m_filePath);
        QCOMPARE(store.contextSize(), QStringLiteral("131072"));
        QVERIFY(store.useMmap());
    }

    void editsEmitKeyAndWriteAfterDebounce() {
        SettingsStore store(m_filePath);
        QSignalSpy changedSpy(&store, &SettingsStore::changed);
        store.setPort(QStringLiteral("9090"));
        store.setPort(QStringLiteral("9090"));
        QCOMPARE(changedSpy.count(), 1);
        QCOMPARE(changedSpy.first().first().toString(), QStringLiteral("port"));
        QVERIFY(!QFile::exists(m_filePath));
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(m_filePath), SettingsStore::SAVE_DEBOUNCE_MS * 3);
        QCOMPARE(readJson(m_filePath).value(QStringLiteral("port")), QJsonValue(QStringLiteral("9090")));
    }

    void gpuChipsFollowChipOrder() {
        SettingsStore store(m_filePath);
        const QVariantList chipOrder{0, 1, 2};
        store.setGpus(QStringLiteral("1, 0"));
        QCOMPARE(store.gpuOrder(1), 0);
        QCOMPARE(store.gpuOrder(2), -1);
        store.toggleGpu(2, chipOrder);
        QCOMPARE(store.gpus(), QStringLiteral("0,1,2"));
        store.toggleGpu(0, chipOrder);
        QCOMPARE(store.gpus(), QStringLiteral("1,2"));
        QVERIFY(store.isGpuListValid(QStringLiteral("0,1")));
        QVERIFY(!store.isGpuListValid(QStringLiteral("0;1")));
        QVERIFY(!store.isGpuListValid(QString()));
    }

    void configurationRoundTripsWithoutCaches() {
        SettingsStore store(m_filePath);
        store.setContextSize(QStringLiteral("8192"));
        const QJsonObject configuration = store.configuration();
        QVERIFY(!configuration.contains(QStringLiteral("server_cache")));
        QCOMPARE(configuration.size(), 17);
        store.setContextSize(QStringLiteral("200000"));
        store.applyConfiguration(configuration);
        QCOMPARE(store.contextSize(), QStringLiteral("8192"));
    }

    void appWideKeysRoundTripAndKeepTheRest() {
        QVERIFY(QFile::copy(QStringLiteral(GIGA_STRATA_APP_FIXTURES "/real_launcher_settings.json"), m_filePath));
        QFile::setPermissions(m_filePath, QFile::ReadOwner | QFile::WriteOwner);
        {
            SettingsStore store(m_filePath);
            QVERIFY(store.serveOnLan());  // the Python host 0.0.0.0
            QVERIFY(store.modelFolders().isEmpty());
            QCOMPARE(store.reduceMotion(), QStringLiteral("system"));
            QVERIFY(store.keepServingInTray());
            store.setModelFolders({QStringLiteral("N:\\models")});
            store.setServeOnLan(false);
            store.setReduceMotion(QStringLiteral("on"));
            store.setLogDrawerHeight(300);
            store.setParametersOpen(true);
        }
        const QJsonObject saved = readJson(m_filePath);
        QCOMPARE(saved.value(QStringLiteral("host")).toString(), QStringLiteral("127.0.0.1"));
        QCOMPARE(saved.value(QStringLiteral("model_folders")).toArray().first().toString(), QStringLiteral("N:\\models"));
        QVERIFY(saved.contains(QStringLiteral("server_cache")));
        const SettingsStore reread(m_filePath);
        QVERIFY(!reread.serveOnLan());
        QCOMPARE(reread.reduceMotion(), QStringLiteral("on"));
        QCOMPARE(reread.logDrawerHeight(), 300.0);
        QVERIFY(reread.isParametersOpen());
        QCOMPARE(reread.port(), QStringLiteral("1234"));
    }
};

QTEST_GUILESS_MAIN(TestSettingsStore)
#include "test_settings_store.moc"

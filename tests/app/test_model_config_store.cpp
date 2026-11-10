#include "model_config_store.h"
#include "settings_file.h"

#include <QFile>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

// Every test works in a temporary folder injected through the constructor, never %APPDATA%.
class TestModelConfigStore : public QObject {
    Q_OBJECT

private:
    static QString model() { return QStringLiteral("N:\\models\\GLM-5.3-q4-00001-of-00004.gguf"); }

    static QJsonObject readJson(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
    }

private slots:
    void defaultsUseEveryDetectedGpu() {
        QTemporaryDir folder;
        ModelConfigStore store(folder.path(), [] { return QVector<int>{0, 1}; });
        const QVariantMap defaults = store.defaults();
        QCOMPARE(defaults.value(QStringLiteral("gpus")).toString(), QStringLiteral("0,1"));
        QCOMPARE(defaults.value(QStringLiteral("ctx")).toString(), QStringLiteral("32768"));
        QCOMPARE(defaults.value(QStringLiteral("ubatch")).toString(), QStringLiteral("auto"));
        QVERIFY(defaults.value(QStringLiteral("mtp")).toBool());
        QVERIFY(!defaults.value(QStringLiteral("per_expert")).toBool());
        QCOMPARE(store.load(model()), defaults);
        ModelConfigStore noGpus(folder.path(), [] { return QVector<int>(); });
        QCOMPARE(noGpus.defaults().value(QStringLiteral("gpus")).toString(), QString());
    }

    void fileIsNamedByStemAndKeepsTypesAndUnknownKeys() {
        QTemporaryDir folder;
        ModelConfigStore store(folder.path(), [] { return QVector<int>{0}; });
        const QString path = store.configPath(model());
        QCOMPARE(path, QDir::toNativeSeparators(folder.filePath(QStringLiteral("GLM-5.3-q4.json"))));
        QFile seed(path);
        QVERIFY(seed.open(QIODevice::WriteOnly));
        seed.write(R"({"ctx": 200000, "note": "mine"})");
        seed.close();
        QCOMPARE(store.load(model()).value(QStringLiteral("ctx")).toString(), QStringLiteral("200000"));
        QVariantMap values = store.load(model());
        values.insert(QStringLiteral("per_expert"), true);
        values.insert(QStringLiteral("margin"), 2048);
        QSignalSpy changed(&store, &ModelConfigStore::configChanged);
        QVERIFY(store.save(model(), values));
        QCOMPARE(changed.count(), 1);
        const QJsonObject saved = readJson(path);
        QCOMPARE(saved.value(QStringLiteral("note")).toString(), QStringLiteral("mine"));
        QVERIFY(saved.value(QStringLiteral("margin")).isString());
        QCOMPARE(saved.value(QStringLiteral("margin")).toString(), QStringLiteral("2048"));
        QVERIFY(saved.value(QStringLiteral("per_expert")).isBool());
        QVERIFY(store.load(model()).value(QStringLiteral("per_expert")).toBool());
    }

    void migrationSeedsOnlyAMissingFile() {
        QTemporaryDir folder;
        ModelConfigStore store(folder.path(), [] { return QVector<int>{0}; });
        const auto legacy = SettingsFile::read(QStringLiteral(GIGA_STRATA_APP_FIXTURES "/real_launcher_settings.json"));
        QVERIFY(legacy.ok());
        store.migrate(legacy.value());
        const QString legacyModel = legacy.value().value(QStringLiteral("model")).toString();
        const QVariantMap migrated = store.load(legacyModel);
        QCOMPARE(migrated.value(QStringLiteral("ctx")).toString(), QStringLiteral("200000"));
        QCOMPARE(migrated.value(QStringLiteral("gpus")).toString(), QStringLiteral("0,1"));
        QVERIFY(migrated.value(QStringLiteral("mtp")).toBool());
        QVariantMap edited = migrated;
        edited.insert(QStringLiteral("ctx"), QStringLiteral("8192"));
        QVERIFY(store.save(legacyModel, edited));
        store.migrate(legacy.value());
        QCOMPARE(store.load(legacyModel).value(QStringLiteral("ctx")).toString(), QStringLiteral("8192"));
    }

    void outsideEditIsReported() {
        QTemporaryDir folder;
        ModelConfigStore store(folder.path(), [] { return QVector<int>{0}; });
        QVERIFY(store.ensureFile(model()));
        store.load(model());
        QSignalSpy changed(&store, &ModelConfigStore::configChanged);
        // QFileSystemWatcher arms its Windows change notification on a worker thread; a write in
        // the first instant after load() can land before it is armed (seen as a 1-in-3 flake
        // under the full suite). A person editing config.json is never that fast.
        QTest::qWait(300);
        QFile file(store.configPath(model()));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(R"({"ctx": "4096"})");
        file.close();
        QVERIFY(changed.wait(3000));
        QCOMPARE(changed.first().first().toString(), model());
    }

    void removeDeletesTheFile() {
        QTemporaryDir folder;
        ModelConfigStore store(folder.path(), [] { return QVector<int>{0}; });
        QVERIFY(store.ensureFile(model()));
        QVERIFY(QFile::exists(store.configPath(model())));
        QVERIFY(store.remove(model()));
        QVERIFY(!QFile::exists(store.configPath(model())));
        QVERIFY(store.remove(model()));
    }
};

QTEST_GUILESS_MAIN(TestModelConfigStore)
#include "test_model_config_store.moc"

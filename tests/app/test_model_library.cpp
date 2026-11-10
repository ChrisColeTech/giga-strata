#include "model_config_store.h"
#include "model_library.h"
#include "settings_store.h"

#include "synthetic_model.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

// A temporary model folder with a model, its MTP draft and projector, a split set and a broken
// file. Shell actions are recorded, never run.
class TestModelLibrary : public QObject {
    Q_OBJECT

private:
    struct World {
        QTemporaryDir folder;
        QStringList started;
        QList<QUrl> opened;
        std::unique_ptr<SettingsStore> settings;
        std::unique_ptr<ModelConfigStore> configs;
        std::unique_ptr<ModelLibrary> library;
        QString big;
        QString split;

        World() {
            const QString models = folder.filePath(QStringLiteral("models"));
            big = SyntheticModel::write(models + QStringLiteral("/a/Big-Q4_K_M.gguf"),
                                        SyntheticModel::moe(QStringLiteral("testmoe"), 4));
            SyntheticModel::write(models + QStringLiteral("/a/Big-mtp.gguf"), SyntheticModel::moe(QStringLiteral("testmoe"), 5, 1));
            SyntheticModel::write(models + QStringLiteral("/a/mmproj-big.gguf"), SyntheticModel::projector());
            split = SyntheticModel::write(models + QStringLiteral("/b/Split-IQ3_S-00001-of-00002.gguf"),
                                          SyntheticModel::moe(QStringLiteral("other"), 2, 0, 9999));
            SyntheticModel::write(models + QStringLiteral("/b/Split-IQ3_S-00002-of-00002.gguf"),
                                  SyntheticModel::moe(QStringLiteral("other"), 2, 0, 9999));
            SyntheticModel::write(models + QStringLiteral("/b/broken.gguf"), QByteArray("not a gguf"));
            settings = std::make_unique<SettingsStore>(folder.filePath(QStringLiteral("launcher_settings.json")));
            configs = std::make_unique<ModelConfigStore>(folder.filePath(QStringLiteral("configs")),
                                                         [] { return QVector<int>{0}; });
            ShellActions shell;
            shell.startDetached = [this](const QString &program, const QString &arguments) {
                started.append(program + u' ' + arguments);
                return true;
            };
            shell.openUrl = [this](const QUrl &url) {
                opened.append(url);
                return true;
            };
            library = std::make_unique<ModelLibrary>(*settings, *configs, folder.filePath(QStringLiteral("moe_usage")), shell);
        }

        bool scan() {
            QSignalSpy done(library.get(), &ModelLibrary::modelCountChanged);
            library->addFolder(QUrl::fromLocalFile(folder.filePath(QStringLiteral("models"))));
            while (done.wait(5000)) {
                if (!library->isScanning() && library->modelCount() > 0) {
                    return true;
                }
            }
            return false;
        }

        QVariant cell(int row, const char *role) const {
            QAbstractItemModel *table = library->tableModel();
            return table->index(row, 0).data(table->roleNames().key(role));
        }
    };

private slots:
    void scanListsModelsNotDraftsProjectorsOrShards() {
        World world;
        QVERIFY(world.scan());
        QCOMPARE(world.library->modelCount(), 2);
        QCOMPARE(world.cell(0, "name").toString(), QStringLiteral("Big-Q4_K_M"));
        QCOMPARE(world.cell(0, "quant").toString(), QStringLiteral("Q4_K_M"));
        QCOMPARE(world.cell(0, "type").toString(), QStringLiteral("testmoe"));
        QCOMPARE(world.cell(0, "experts").toInt(), 8);
        QCOMPARE(world.cell(0, "files").toInt(), 1);
        QVERIFY(world.cell(0, "hasDraft").toBool());
        QVERIFY(world.cell(0, "hasProjector").toBool());
        QVERIFY(!world.cell(0, "hasUsage").toBool());
        QCOMPARE(world.cell(1, "name").toString(), QStringLiteral("Split-IQ3_S"));
        QCOMPARE(world.cell(1, "quant").toString(), QStringLiteral("IQ3_S"));
        QCOMPARE(world.cell(1, "files").toInt(), 2);
        QCOMPARE(world.cell(1, "sizeBytes").toDouble(), 2.0 * QFileInfo(world.split).size());
        QCOMPARE(world.library->folders().size(), 1);
    }

    void sortsAndFilters() {
        World world;
        QVERIFY(world.scan());
        world.library->sortBy(QStringLiteral("files"), false);
        QCOMPARE(world.cell(0, "name").toString(), QStringLiteral("Split-IQ3_S"));
        QCOMPARE(world.library->filterValues(QStringLiteral("type")), QStringList({QStringLiteral("other"), QStringLiteral("testmoe")}));
        world.library->setFilter(QStringLiteral("type"), QStringList{QStringLiteral("testmoe")});
        QCOMPARE(world.library->tableModel()->rowCount(), 1);
        QCOMPARE(world.library->activeFilters(), QStringList{QStringLiteral("type")});
        world.library->setFilter(QStringLiteral("name"), QStringLiteral("split"));
        QCOMPARE(world.library->tableModel()->rowCount(), 0);
        world.library->clearFilters();
        QCOMPARE(world.library->tableModel()->rowCount(), 2);
        QVERIFY(world.library->activeFilters().isEmpty());
    }

    void rowActionsUseTheShell() {
        World world;
        QVERIFY(world.scan());
        world.library->revealInExplorer(world.big);
        QCOMPARE(world.started, QStringList{QStringLiteral("explorer.exe /select,\"%1\"").arg(world.big)});
        world.library->openConfigFile(world.big);
        QCOMPARE(world.opened, QList<QUrl>{QUrl::fromLocalFile(world.configs->configPath(world.big))});
        QVERIFY(QFile::exists(world.configs->configPath(world.big)));
    }

    void deleteRemovesEveryShardAndTheConfig() {
        World world;
        QVERIFY(world.scan());
        QVERIFY(world.configs->ensureFile(world.split));
        QSignalSpy deleted(world.library.get(), &ModelLibrary::modelDeleted);
        world.library->deleteModel(world.split);
        QCOMPARE(deleted.count(), 1);
        QVERIFY(!QFile::exists(world.split));
        QVERIFY(!QFile::exists(QString(world.split).replace(QStringLiteral("00001-of"), QStringLiteral("00002-of"))));
        QVERIFY(!QFile::exists(world.configs->configPath(world.split)));
        QCOMPARE(world.library->modelCount(), 1);
    }
};

QTEST_GUILESS_MAIN(TestModelLibrary)
#include "test_model_library.moc"

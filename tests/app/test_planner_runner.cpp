#include "model_config_store.h"
#include "planner_runner.h"
#include "server_build.h"
#include "settings_store.h"

#include "fake_server_environment.h"
#include "synthetic_model.h"

#include "gguf/shard_cache.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

// Plans a small synthetic MoE model with an injected hardware reading; the build does not exist,
// so the live preview assumes every flag (02 D3) and nothing is ever started.
class TestPlannerRunner : public QObject {
    Q_OBJECT

private:
    struct World {
        QTemporaryDir folder;
        FakeServerEnvironment environment;
        ServerBuild build{environment, QStringLiteral("C:\\no-such-dir\\llama-server.exe")};
        ShardCache shards;
        std::unique_ptr<SettingsStore> settings;
        std::unique_ptr<ModelConfigStore> configs;
        std::unique_ptr<PlannerRunner> runner;
        std::optional<HardwareSnapshot> hardware;
        QString model;

        World() {
            hardware = HardwareSnapshot{{Gpu{0, QStringLiteral("Test GPU"), 8192, 16384}}, qint64(64) << 30};
            model = SyntheticModel::write(folder.filePath(QStringLiteral("models/Test-MoE-Q4_K_M.gguf")),
                                          SyntheticModel::moe(QStringLiteral("testmoe"), 4));
            settings = std::make_unique<SettingsStore>(folder.filePath(QStringLiteral("launcher_settings.json")));
            configs = std::make_unique<ModelConfigStore>(folder.filePath(QStringLiteral("configs")),
                                                         [] { return QVector<int>{0}; });
            const PlanningContext context{&build, &shards, folder.filePath(QStringLiteral("models")),
                                          folder.filePath(QStringLiteral("moe_usage")),
                                          folder.filePath(QStringLiteral("moe_usage/hot"))};
            runner = std::make_unique<PlannerRunner>(*configs, *settings, context, [this] { return hardware; });
        }
    };

private slots:
    void plansTheModelWithItsSavedParameters() {
        World world;
        QSignalSpy planned(world.runner.get(), &PlannerRunner::planChanged);
        world.runner->setModelPath(world.model);
        QVERIFY(planned.wait(5000));
        QVERIFY2(world.runner->errorText().isEmpty(), qPrintable(world.runner->errorText()));
        QCOMPARE(world.runner->summary(), QStringLiteral("4 layers: 4 on GPU0"));
        QVERIFY(world.runner->reportLines().first().startsWith(QStringLiteral("Model: testmoe, 4 layers, 8 experts")));
        const QVariantList tiers = world.runner->tiers();
        QCOMPARE(tiers.size(), 2);
        QCOMPARE(tiers[0].toMap().value(QStringLiteral("label")).toString(), QStringLiteral("GPU0"));
        QCOMPARE(tiers[0].toMap().value(QStringLiteral("capacityBytes")).toDouble(), 8192.0 * 1024 * 1024);
        QVERIFY(tiers[0].toMap().value(QStringLiteral("usedBytes")).toDouble() > 0);
        QCOMPARE(tiers[1].toMap().value(QStringLiteral("label")).toString(), QStringLiteral("RAM"));
        const QVariantMap info = world.runner->modelInfo();
        QCOMPARE(info.value(QStringLiteral("type")).toString(), QStringLiteral("testmoe"));
        QCOMPARE(info.value(QStringLiteral("layers")).toInt(), 4);
        QCOMPARE(info.value(QStringLiteral("files")).toInt(), 1);
        QVERIFY(!world.runner->isPlanning());
    }

    void editsAreDebouncedIntoOnePlan() {
        World world;
        QSignalSpy planned(world.runner.get(), &PlannerRunner::planChanged);
        world.runner->setModelPath(world.model);
        QVERIFY(planned.wait(5000));
        planned.clear();
        world.runner->setParameter(QStringLiteral("ctx"), QStringLiteral("8192"));
        world.runner->setParameter(QStringLiteral("ctx"), QStringLiteral("16384"));
        world.runner->setParameter(QStringLiteral("margin"), QStringLiteral("512"));
        QVERIFY(world.runner->isDirty());
        QVERIFY(planned.wait(5000));
        QTest::qWait(PlannerRunner::DEBOUNCE_MS + 200);
        QCOMPARE(planned.count(), 1);
        QVERIFY(world.runner->reportLines().join(u'\n').contains(QStringLiteral("CUDA0 Test GPU")));
    }

    void badValuesAreReportedInWords() {
        World world;
        QSignalSpy planned(world.runner.get(), &PlannerRunner::planChanged);
        world.runner->setModelPath(world.model);
        QVERIFY(planned.wait(5000));
        world.runner->setParameter(QStringLiteral("gpus"), QStringLiteral("7"));
        QVERIFY(planned.wait(5000));
        QCOMPARE(world.runner->errorText(), QStringLiteral("GPU id(s) [7] not found; nvidia-smi reports [0]"));
        QVERIFY(world.runner->tiers().isEmpty());
        world.runner->setParameter(QStringLiteral("ctx"), QStringLiteral("lots"));
        QVERIFY(planned.wait(5000));
        QVERIFY(world.runner->errorText().startsWith(QStringLiteral("GPU IDs, context size")));
    }

    void waitsForTheFirstHardwareReading() {
        World world;
        world.hardware.reset();
        QSignalSpy planned(world.runner.get(), &PlannerRunner::planChanged);
        world.runner->setModelPath(world.model);
        QVERIFY(!planned.wait(PlannerRunner::DEBOUNCE_MS + 300));
        world.hardware = HardwareSnapshot{{Gpu{0, QStringLiteral("Test GPU"), 8192, 16384}}, std::nullopt};
        world.runner->onHardwareChanged();
        QVERIFY(planned.wait(5000));
        QVERIFY(world.runner->errorText().isEmpty());
    }

    void saveWritesTheConfigAndResetRestoresDefaults() {
        World world;
        world.runner->setModelPath(world.model);
        world.runner->setParameter(QStringLiteral("ctx"), QStringLiteral("8192"));
        QVERIFY(world.runner->save());
        QVERIFY(!world.runner->isDirty());
        QCOMPARE(world.configs->load(world.model).value(QStringLiteral("ctx")).toString(), QStringLiteral("8192"));
        world.runner->resetToDefaults();
        QCOMPARE(world.runner->parameters().value(QStringLiteral("ctx")).toString(), QStringLiteral("32768"));
        QVERIFY(world.runner->isDirty());
    }
};

QTEST_GUILESS_MAIN(TestPlannerRunner)
#include "test_planner_runner.moc"

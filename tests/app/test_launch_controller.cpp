#include "launch_controller.h"
#include "log_model.h"
#include "model_config_store.h"
#include "server_build.h"
#include "settings_store.h"

#include "synthetic_model.h"

#include "gguf/shard_cache.h"
#include "launch/command_line.h"
#include "launch/local_server_environment.h"
#include "launch/server_exit.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

// Runs the whole Run path against tests/tools/fake_server, which replays captured llama-server
// logs (tests/golden/fixtures/logs). No real llama-server, no model load, no GPU.
class TestLaunchController : public QObject {
    Q_OBJECT

private:
    struct World {
        QTemporaryDir folder;
        LocalServerEnvironment environment;
        ServerBuild build{environment, QStringLiteral("C:\\no-such-dir\\llama-server.exe")};
        ShardCache shards;
        LogModel log{nullptr};
        std::unique_ptr<SettingsStore> settings;
        std::unique_ptr<ModelConfigStore> configs;
        std::unique_ptr<LaunchController> launcher;
        QString model;

        World() {
            model = SyntheticModel::write(folder.filePath(QStringLiteral("models/Test-MoE.gguf")),
                                          SyntheticModel::moe(QStringLiteral("testmoe"), 4));
            settings = std::make_unique<SettingsStore>(folder.filePath(QStringLiteral("launcher_settings.json")));
            settings->setServerOverride(QCoreApplication::applicationDirPath() + QStringLiteral("/fake_server.exe"));
            settings->setServeOnLan(false);
            settings->setPort(QStringLiteral("8123"));
            configs = std::make_unique<ModelConfigStore>(folder.filePath(QStringLiteral("configs")),
                                                         [] { return QVector<int>{0}; });
            const PlanningContext context{&build, &shards, folder.filePath(QStringLiteral("models")),
                                          folder.filePath(QStringLiteral("moe_usage")),
                                          folder.filePath(QStringLiteral("moe_usage/hot"))};
            const auto hardware = [] {
                return HardwareSnapshot{{Gpu{0, QStringLiteral("Test GPU"), 8192, 16384}}, qint64(64) << 30};
            };
            launcher = std::make_unique<LaunchController>(*configs, *settings, context, hardware, log);
        }

        bool waitFor(LaunchController::State state) {
            QSignalSpy changed(launcher.get(), &LaunchController::stateChanged);
            while (launcher->state() != state) {
                if (!changed.wait(10000)) {
                    return false;
                }
            }
            return true;
        }
    };

    static void replay(const char *log, bool isHeld, quint32 exitCode = 0) {
        qputenv("GIGA_STRATA_FAKE_LOG", QByteArray(GIGA_STRATA_GOLDEN "/fixtures/logs/") + log);
        qputenv("GIGA_STRATA_FAKE_HOLD", isHeld ? "1" : "0");
        qputenv("GIGA_STRATA_FAKE_EXIT", QByteArray::number(exitCode));
    }

private slots:
    void servesReportsSpeedAndStops() {
        World world;
        replay("serve_cpu_upstream_qwen3-8b.log", true);
        const QString argsFile = world.folder.filePath(QStringLiteral("args.txt"));
        qputenv("GIGA_STRATA_FAKE_ARGS", argsFile.toLocal8Bit());
        world.launcher->serve(world.model);
        QCOMPARE(world.launcher->state(), LaunchController::State::Starting);
        QVERIFY(world.waitFor(LaunchController::State::Serving));
        QCOMPARE(world.launcher->url(), QStringLiteral("http://127.0.0.1:8123"));
        QCOMPARE(world.launcher->modelName(), QStringLiteral("Test-MoE"));
        QTRY_VERIFY_WITH_TIMEOUT(world.launcher->generationTokensPerSecond() > 0, 10000);
        QVERIFY(world.launcher->promptTokensPerSecond() > 0);
        const QString logText = world.log.visibleText().join(u'\n');
        QVERIFY(logText.contains(QStringLiteral("[PLANNER] Model: testmoe")));
        QVERIFY(logText.contains(QStringLiteral("[LAUNCHER] Executing command: ")));
        QVERIFY(logText.contains(QStringLiteral("listening on http://127.0.0.1:50582")));
        QFile args(argsFile);
        QVERIFY(args.open(QIODevice::ReadOnly));
        const QString commandLine = QString::fromUtf8(args.readAll());
        QVERIFY2(commandLine.contains(QStringLiteral(" -m ") + CommandLine::join({world.model}) + QStringLiteral(" -ngl 999 -c 32768")),
                 qPrintable(commandLine));
        QVERIFY(commandLine.contains(QStringLiteral("--host 127.0.0.1 --port 8123 --alias Test-MoE --fit off")));
        world.launcher->stop();
        QVERIFY(world.waitFor(LaunchController::State::Stopped));
        QVERIFY(world.launcher->failureReason().isEmpty());
        QVERIFY(world.launcher->url().isEmpty());
        qunsetenv("GIGA_STRATA_FAKE_ARGS");
    }

    void loadFailureIsExplained() {
        World world;
        replay("probe_fail_upstream_unknown_arch.log", false, 1);
        world.launcher->serve(world.model);
        QVERIFY(world.waitFor(LaunchController::State::Failed));
        QVERIFY2(world.launcher->failureReason().startsWith(QStringLiteral("The server could not load the model: ")),
                 qPrintable(world.launcher->failureReason()));
        QVERIFY(world.log.visibleText().contains(QStringLiteral("[LAUNCHER] Server exited with code 1")));
    }

    void missingDllGetsTheHint() {
        World world;
        replay("missing_dll_lmstudio.log", false, 0xC0000135);
        world.launcher->serve(world.model);
        QVERIFY(world.waitFor(LaunchController::State::Failed));
        QCOMPARE(world.launcher->failureReason(), ServerExit::missingDllHint());
    }

    void serveReplacesTheRunningServer() {
        World world;
        const QString second = SyntheticModel::write(world.folder.filePath(QStringLiteral("models/Second.gguf")),
                                                     SyntheticModel::moe(QStringLiteral("testmoe"), 2));
        replay("serve_cpu_upstream_qwen3-8b.log", true);
        world.launcher->serve(world.model);
        QVERIFY(world.waitFor(LaunchController::State::Serving));
        world.launcher->serve(second);
        QVERIFY(world.waitFor(LaunchController::State::Stopping));
        QVERIFY(world.waitFor(LaunchController::State::Serving));
        QCOMPARE(world.launcher->modelName(), QStringLiteral("Second"));
        world.launcher->stop();
        QVERIFY(world.waitFor(LaunchController::State::Stopped));
    }

    void missingBinaryFailsInWords() {
        World world;
        world.settings->setServerOverride(QString());
        world.launcher->serve(world.model);
        QCOMPARE(world.launcher->state(), LaunchController::State::Failed);
        QVERIFY(world.launcher->failureReason().startsWith(QStringLiteral("llama-server was not found at C:\\no-such-dir")));
    }

    void planFailureNeverStartsAProcess() {
        World world;
        QVariantMap values = world.configs->load(world.model);
        values.insert(QStringLiteral("gpus"), QStringLiteral("3"));
        QVERIFY(world.configs->save(world.model, values));
        world.launcher->serve(world.model);
        QVERIFY(world.waitFor(LaunchController::State::Failed));
        QCOMPARE(world.launcher->failureReason(), QStringLiteral("GPU id(s) [3] not found; nvidia-smi reports [0]"));
        QVERIFY(world.log.visibleText().contains(QStringLiteral("[CRITICAL RUNTIME ERROR]: GPU id(s) [3] not found; "
                                                                "nvidia-smi reports [0]")));
    }
};

QTEST_GUILESS_MAIN(TestLaunchController)
#include "test_launch_controller.moc"

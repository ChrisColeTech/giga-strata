#include "common/paths.h"
#include "gguf/shard_cache.h"
#include "launch/local_server_environment.h"
#include "launch_controller.h"
#include "log_model.h"
#include "model_config_store.h"
#include "model_library.h"
#include "monitor_controller.h"
#include "planner_runner.h"
#include "server_build.h"
#include "settings_store.h"
#include "shell_actions.h"
#include "single_instance.h"
#include "system_info.h"
#include "tray_host.h"

#include <QApplication>
#include <QColor>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QStyle>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(GigaStrata_AppPlugin)
Q_IMPORT_QML_PLUGIN(GigaStrataPlugin)

namespace {

// %APPDATA%\giga-strata\models (05 section 2); beside the exe when Windows gives no APPDATA.
QString modelConfigDirectory() {
    const QString appData = qEnvironmentVariable("APPDATA");
    const QString base = appData.isEmpty() ? QCoreApplication::applicationDirPath() : appData;
    return QDir::toNativeSeparators(QDir(base).filePath(QStringLiteral("giga-strata/models")));
}

QVector<int> detectedGpuIndices(const MonitorController &monitor) {
    QVector<int> indices;
    if (const auto snapshot = monitor.snapshot()) {
        for (const Gpu &gpu : snapshot->gpus) {
            indices.append(gpu.index);
        }
    }
    return indices;
}

} // namespace

// Composition root (R9): every controller is built here with its real dependencies and handed
// to QML through provide() before the first QML file loads.
int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Giga Strata"));
    QApplication::setOrganizationName(QStringLiteral("giga-strata"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    // FR-6.3: checked before anything else loads, so a second launch exits within milliseconds.
    SingleInstance singleInstance(SingleInstance::keyForInstallation(QCoreApplication::applicationDirPath()));
    if (!singleInstance.claim()) {
        return 0;
    }

    const Paths paths(Paths::findProjectRoot(QCoreApplication::applicationDirPath()));
    ShardCache shards;
    const LocalServerEnvironment serverEnvironment;
    ServerBuild serverBuild(serverEnvironment, QDir::toNativeSeparators(QDir(QCoreApplication::applicationDirPath())
                                                                            .filePath(QStringLiteral("server/llama-server.exe"))));
    const PlanningContext planning{&serverBuild, &shards, paths.modelsDirectory(), paths.usageDirectory(),
                                   paths.hotDirectory()};

    SystemInfo systemInfo(nullptr);
    SystemInfo::provide(&systemInfo);
    SettingsStore settings(paths.settingsFile());
    SettingsStore::provide(&settings);
    MonitorController monitor{HardwareProbe()};
    MonitorController::provide(&monitor);
    ModelConfigStore configs(modelConfigDirectory(), [&monitor] { return detectedGpuIndices(monitor); });
    configs.migrate(settings.document());
    ModelConfigStore::provide(&configs);
    LogModel log(nullptr);
    LogModel::provide(&log);
    PlannerRunner planner(configs, settings, planning, [&monitor] { return monitor.snapshot(); });
    PlannerRunner::provide(&planner);
    QObject::connect(&monitor, &MonitorController::readingChanged, &planner, &PlannerRunner::onHardwareChanged);
    ModelLibrary library(settings, configs, paths.usageDirectory(), ShellActions::system());
    ModelLibrary::provide(&library);
    // Run takes its own reading: VRAM a just-stopped server held must count as free.
    LaunchController launcher(configs, settings, planning, [] { return HardwareProbe().snapshot(); }, log);
    LaunchController::provide(&launcher);

    const QIcon appIcon = TrayHost::appIcon();
    QApplication::setWindowIcon(appIcon);
    TrayHost tray(appIcon);
    TrayHost::provide(&tray);
    launcher.attachTray(tray);
    const auto applyTrayPreferences = [&settings, &tray] {
        tray.setHidePreferences(settings.keepServingInTray(), settings.minimizeToTray());
    };
    applyTrayPreferences();
    QObject::connect(&settings, &SettingsStore::valuesChanged, &tray, applyTrayPreferences);
    QObject::connect(&application, &QCoreApplication::aboutToQuit, &settings, &SettingsStore::flush);
    QObject::connect(&application, &QCoreApplication::aboutToQuit, &monitor, &MonitorController::stop);
    QObject::connect(&application, &QCoreApplication::aboutToQuit, &launcher, &LaunchController::stop);
    QObject::connect(&tray, &TrayHost::quitRequested, &application, &QCoreApplication::quit);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &application, [] { QCoreApplication::exit(1); },
        Qt::QueuedConnection);
    engine.loadFromModule("GigaStrata", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    // Theme owns every color literal (R16); the tray borrows its semantic colors for the N4 dots.
    const QObject *theme = engine.singletonInstance<QObject *>("GigaStrata", "Theme");
    tray.setDotColors(theme->property("success").value<QColor>(), theme->property("danger").value<QColor>());

    // The window is created hidden so the dark title bar is in place before the first frame.
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    SystemInfo::applyDarkTitleBar(window);
    tray.attachWindow(window);
    QObject::connect(&singleInstance, &SingleInstance::activationRequested, &tray, &TrayHost::showWindow);
    // R28: nobody sees the gauges while minimized or in the tray, so nvidia-smi is not polled.
    QObject::connect(window, &QWindow::visibilityChanged, &monitor, [&monitor](QWindow::Visibility visibility) {
        monitor.setPaused(visibility == QWindow::Hidden || visibility == QWindow::Minimized);
    });
    // Opens maximized. The restored size is 80% of the screen's work area, centred: a fixed
    // 1280x800 is 3840x2400 physical pixels at 300% scaling, taller than the screen, and Windows
    // then places it off centre.
    if (QScreen *screen = window->screen()) {
        constexpr double RESTORED_SHARE = 0.8;
        const QRect available = screen->availableGeometry();
        const QSize restored(qMax(window->minimumWidth(), int(available.width() * RESTORED_SHARE)),
                             qMax(window->minimumHeight(), int(available.height() * RESTORED_SHARE)));
        window->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, restored, available));
    }
    window->showMaximized();
    return QApplication::exec();
}

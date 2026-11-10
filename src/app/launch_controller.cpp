#include "launch_controller.h"

#include "launch_preparer.h"
#include "log_model.h"
#include "model_config_store.h"
#include "server_build.h"
#include "settings_store.h"
#include "tray_host.h"

#include "common/model_names.h"
#include "launch/server_exit.h"

#include <QClipboard>
#include <QDir>
#include <QGuiApplication>
#include <QJSEngine>
#include <QtConcurrent/QtConcurrentRun>

#include <utility>

namespace {

constexpr int UPTIME_TICK_MS = 1000;

} // namespace

LaunchController::LaunchController(ModelConfigStore &configs, SettingsStore &settings, PlanningContext context,
                                   HardwareSource hardware, LogModel &log, QObject *parent)
    : QObject(parent), m_configs(configs), m_settings(settings), m_context(std::move(context)),
      m_hardware(std::move(hardware)), m_log(log) {
    m_uptimeTimer.setInterval(UPTIME_TICK_MS);
    connect(&m_uptimeTimer, &QTimer::timeout, this, [this] {
        m_uptimeSeconds = int(m_uptimeClock.elapsed() / 1000);
        emit uptimeChanged();
    });
    connect(&m_planWatcher, &QFutureWatcher<PlanJobResult>::finished, this, &LaunchController::onPlanned);
    connect(&m_process, &ServerProcess::eventRead, this, &LaunchController::onEvent);
    connect(&m_process, &ServerProcess::exited, this, &LaunchController::onExited);
    connect(&m_process, &ServerProcess::failedToStart, this, &LaunchController::fail);
}

LaunchController::~LaunchController() {
    m_planWatcher.waitForFinished();
}

void LaunchController::provide(LaunchController *instance) {
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

LaunchController *LaunchController::create(QQmlEngine *, QJSEngine *) {
    Q_ASSERT_X(s_instance, "LaunchController::create", "main.cpp must call LaunchController::provide() before loading QML");
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

QString LaunchController::stateText() const {
    if (m_state == State::Loading && m_loadProgress >= 0) {
        return tr("Loading %1%").arg(m_loadProgress);
    }
    // in State order
    const QString names[] = {tr("Stopped"), tr("Starting"), tr("Loading"), tr("Serving"), tr("Stopping"), tr("Failed")};
    return names[int(m_state)];
}

void LaunchController::serve(const QString &modelPath) {
    if (modelPath.isEmpty()) {
        return;
    }
    if (m_process.isRunning()) {
        m_pendingModel = modelPath;  // started once the running server has exited
        stop();
        return;
    }
    startServe(modelPath);
}

void LaunchController::stop() {
    if (m_process.isRunning()) {
        m_isStopRequested = true;
        setState(State::Stopping);
        m_process.kill();
        return;
    }
    if (m_state == State::Starting) {
        ++m_serveGeneration;  // the plan in flight is dropped when it arrives
        setState(State::Stopped);
    }
}

void LaunchController::reload() {
    serve(m_modelPath);
}

void LaunchController::copyUrl() const {
    if (!url().isEmpty()) {
        QGuiApplication::clipboard()->setText(url());
    }
}

void LaunchController::attachTray(TrayHost &tray) {
    m_tray = &tray;
    connect(&tray, &TrayHost::stopRequested, this, &LaunchController::stop);
    syncTray();
}

void LaunchController::startServe(const QString &modelPath) {
    m_log.clear();
    m_modelPath = modelPath;
    m_modelName = ModelNames::modelStem(modelPath);
    emit modelChanged();
    resetReadouts();
    m_isStopRequested = false;
    m_binary = m_context.build->binaryFor(m_settings.serverOverride());
    setState(State::Starting);
    if (!m_context.build->exists(m_binary)) {
        fail(tr("llama-server was not found at %1. Rebuild the app with scripts\\build.bat, or choose a "
                        "llama-server in Settings.")
                 .arg(QDir::toNativeSeparators(m_binary)));
        return;
    }
    const PlanJobInput input = PlanJob::input(
        m_context, PlannerRunner::runValues(m_configs.load(modelPath), modelPath, m_settings), m_binary, true);
    m_plannedGeneration = ++m_serveGeneration;
    const HardwareSource hardware = m_hardware;
    m_planWatcher.setFuture(QtConcurrent::run([input, hardware] {
        PlanJobInput withHardware = input;
        withHardware.hardware = hardware();
        return PlanJob::run(withHardware);
    }));
}

void LaunchController::onPlanned() {
    if (m_plannedGeneration != m_serveGeneration || m_state != State::Starting) {
        return;
    }
    const PlanJobResult result = m_planWatcher.result();
    for (const QString &line : result.run.logLines) {
        m_log.appendMessage(line);
    }
    if (!result.run.ok()) {
        // a settings error stops before planning logs anything, so its CRITICAL line is added here
        failRun(result.error, !result.settings);
        return;
    }
    const auto launch = LaunchPreparer::prepare(result, m_binary, *m_context.build, m_context.usageDirectory,
                                                m_context.hotDirectory);
    if (!launch.ok()) {
        failRun(launch.error(), true);
        return;
    }
    for (const QString &line : launch.value().logLines) {
        m_log.appendMessage(line);
    }
    m_url = LaunchPreparer::clientUrl(m_settings.serveOnLan(), result.settings->port);
    m_process.start(launch.value());
}

void LaunchController::onEvent(const LogEvent &event) {
    switch (event.type) {
    case LogEvent::Type::Line:
        m_log.appendLine(event.text, event.kind);
        return;
    case LogEvent::Type::LoadingStarted:
        m_loadProgress = -1;
        setState(State::Loading);
        return;
    case LogEvent::Type::LoadProgress:
        m_loadProgress = event.progressPercent;
        setState(State::Loading);
        emit stateChanged();
        return;
    case LogEvent::Type::Listening:
        m_uptimeClock.start();
        m_uptimeTimer.start();
        setState(State::Serving);
        return;
    case LogEvent::Type::RequestTiming:
        (event.isPrompt ? m_promptTokensPerSecond : m_generationTokensPerSecond) = event.tokensPerSecond;
        emit speedChanged();
        return;
    default:
        return;  // failures are read from the summary when the process exits
    }
}

void LaunchController::onExited(quint32 exitCode) {
    m_uptimeTimer.stop();
    for (const QString &line : ServerExit::launcherLines(exitCode)) {
        m_log.appendMessage(line);
    }
    const ServerOutcome outcome = ServerExit::interpret(exitCode, m_isStopRequested, m_process.summary());
    if (outcome.isFailure) {
        fail(outcome.message);
    } else {
        setState(State::Stopped);
    }
    if (!m_pendingModel.isEmpty()) {
        const QString next = std::exchange(m_pendingModel, QString());
        startServe(next);
    }
}

void LaunchController::failRun(const QString &error, bool isLogged) {
    if (isLogged) {
        m_log.appendMessage(RunPlanner::criticalLine(error));
    }
    fail(error);
}

void LaunchController::fail(const QString &reason) {
    m_uptimeTimer.stop();
    m_failureReason = reason;
    setState(State::Failed);
}

void LaunchController::setState(State state) {
    if (state != State::Failed) {
        m_failureReason.clear();
    }
    if (m_state == state && state != State::Failed) {
        return;
    }
    m_state = state;
    emit stateChanged();
    syncTray();
}

void LaunchController::resetReadouts() {
    m_loadProgress = -1;
    m_url.clear();
    m_generationTokensPerSecond = 0.0;
    m_promptTokensPerSecond = 0.0;
    m_uptimeSeconds = 0;
    emit speedChanged();
    emit uptimeChanged();
}

void LaunchController::syncTray() {
    if (!m_tray) {
        return;
    }
    m_tray->setServing(m_state == State::Starting || m_state == State::Loading || m_state == State::Serving);
    m_tray->setFailed(m_state == State::Failed);
    const QString status = m_state == State::Serving ? stateText() + QStringLiteral(" - ") + m_url : stateText();
    m_tray->setStatusText(m_modelName.isEmpty() ? status : m_modelName + QStringLiteral("\n") + status);
}

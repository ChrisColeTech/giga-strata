#pragma once

#include "plan_job.h"
#include "planner_runner.h"
#include "server_process.h"

#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <functional>

class LogModel;
class ModelConfigStore;
class QJSEngine;
class QQmlEngine;
class SettingsStore;
class TrayHost;

// The server (05 section 4, 04 sections 5 and 6): plans the model's saved config on the Run path,
// writes what planning deferred, starts the bundled (or override) llama-server and turns its
// output into state, load progress, URL and tok/s. serve() replaces whatever runs without
// asking; stop() kills (02 D6). Failures carry one plain sentence from ServerExit.
class LaunchController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString stateText READ stateText NOTIFY stateChanged)
    Q_PROPERTY(QString failureReason READ failureReason NOTIFY stateChanged)
    Q_PROPERTY(int loadProgress READ loadProgress NOTIFY stateChanged)
    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelChanged)
    Q_PROPERTY(QString modelName READ modelName NOTIFY modelChanged)
    Q_PROPERTY(QString url READ url NOTIFY stateChanged)
    Q_PROPERTY(double generationTokensPerSecond READ generationTokensPerSecond NOTIFY speedChanged)
    Q_PROPERTY(double promptTokensPerSecond READ promptTokensPerSecond NOTIFY speedChanged)
    Q_PROPERTY(int uptimeSeconds READ uptimeSeconds NOTIFY uptimeChanged)

public:
    enum class State { Stopped, Starting, Loading, Serving, Stopping, Failed };
    Q_ENUM(State)

    // A fresh reading for Run, taken on the worker: VRAM a just-stopped server held must count as free.
    using HardwareSource = std::function<HardwareSnapshot()>;

    LaunchController(ModelConfigStore &configs, SettingsStore &settings, PlanningContext context,
                     HardwareSource hardware, LogModel &log, QObject *parent = nullptr);
    ~LaunchController() override;

    static void provide(LaunchController *instance);
    static LaunchController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    State state() const { return m_state; }
    QString stateText() const;
    const QString &failureReason() const { return m_failureReason; }
    int loadProgress() const { return m_loadProgress; }
    const QString &modelPath() const { return m_modelPath; }
    const QString &modelName() const { return m_modelName; }
    QString url() const { return m_state == State::Serving ? m_url : QString(); }
    double generationTokensPerSecond() const { return m_generationTokensPerSecond; }
    double promptTokensPerSecond() const { return m_promptTokensPerSecond; }
    int uptimeSeconds() const { return m_uptimeSeconds; }

    Q_INVOKABLE void serve(const QString &modelPath);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void reload();
    Q_INVOKABLE void copyUrl() const;

    // The tray icon follows the state; its Stop item stops the server.
    void attachTray(TrayHost &tray);

signals:
    void stateChanged();
    void modelChanged();
    void speedChanged();
    void uptimeChanged();

private:
    void startServe(const QString &modelPath);
    void onPlanned();
    void onEvent(const LogEvent &event);
    void onExited(quint32 exitCode);
    void fail(const QString &reason);
    // A Run failure: `isLogged` adds run_process's CRITICAL line to the log first.
    void failRun(const QString &error, bool isLogged);
    void setState(State state);
    void resetReadouts();
    void syncTray();

    static inline LaunchController *s_instance = nullptr;

    ModelConfigStore &m_configs;
    SettingsStore &m_settings;
    PlanningContext m_context;
    HardwareSource m_hardware;
    LogModel &m_log;
    TrayHost *m_tray = nullptr;
    ServerProcess m_process;
    QFutureWatcher<PlanJobResult> m_planWatcher;
    quint64 m_serveGeneration = 0;
    quint64 m_plannedGeneration = 0;
    QString m_binary;
    QString m_pendingModel;
    bool m_isStopRequested = false;
    State m_state = State::Stopped;
    QString m_failureReason;
    int m_loadProgress = -1;
    QString m_modelPath;
    QString m_modelName;
    QString m_url;
    double m_generationTokensPerSecond = 0.0;
    double m_promptTokensPerSecond = 0.0;
    int m_uptimeSeconds = 0;
    QElapsedTimer m_uptimeClock;
    QTimer m_uptimeTimer;
};

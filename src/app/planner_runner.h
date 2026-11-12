#pragma once

#include "plan_job.h"

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <functional>

class ModelConfigStore;
class QJSEngine;
class QQmlEngine;
class SettingsStore;

// Live plan for the model being edited (05 section 3, 01 section 2): every parameter edit
// restarts a 400 ms debounce; then a generation counter is bumped, the previous plan's cancel
// token is set (02 D5) and the Run path plans on a worker. Only the latest generation's result
// is shown. Hardware is MonitorController's latest reading; without one the plan waits for it.
class PlannerRunner : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(QVariantMap parameters READ parameters NOTIFY parametersChanged)
    Q_PROPERTY(bool isDirty READ isDirty NOTIFY parametersChanged)
    Q_PROPERTY(bool isPlanning READ isPlanning NOTIFY isPlanningChanged)
    Q_PROPERTY(QVariantList tiers READ tiers NOTIFY planChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY planChanged)
    Q_PROPERTY(QStringList reportLines READ reportLines NOTIFY planChanged)
    Q_PROPERTY(QStringList warnings READ warnings NOTIFY planChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY planChanged)
    Q_PROPERTY(QVariantMap modelInfo READ modelInfo NOTIFY planChanged)

public:
    using HardwareSource = std::function<std::optional<HardwareSnapshot>()>;
    static constexpr int DEBOUNCE_MS = 400;

    PlannerRunner(ModelConfigStore &configs, SettingsStore &settings, PlanningContext context,
                  HardwareSource hardware, QObject *parent = nullptr);
    ~PlannerRunner() override;

    static void provide(PlannerRunner *instance);
    static PlannerRunner *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    const QString &modelPath() const { return m_modelPath; }
    void setModelPath(const QString &modelPath);
    const QVariantMap &parameters() const { return m_parameters; }
    bool isDirty() const { return m_parameters != m_saved; }
    bool isPlanning() const { return m_isPlanning; }
    const QVariantList &tiers() const { return m_result.tiers; }
    const QString &summary() const { return m_result.summary; }
    QStringList reportLines() const;
    QStringList warnings() const;
    const QString &errorText() const { return m_result.error; }
    const QVariantMap &modelInfo() const { return m_result.modelInfo; }

    Q_INVOKABLE void setParameter(const QString &key, const QVariant &value);
    Q_INVOKABLE void resetToDefaults();
    // Puts the parameters back to what the model's config file holds.
    Q_INVOKABLE void discard();
    Q_INVOKABLE bool save();

    // The values a Run of `modelPath` uses: its saved parameters plus the app-wide ones.
    static QVariantMap runValues(const QVariantMap &parameters, const QString &modelPath, const SettingsStore &settings);

public slots:
    // MonitorController.readingChanged: a plan that waited for the first reading starts now.
    void onHardwareChanged();

signals:
    void modelPathChanged();
    void parametersChanged();
    void isPlanningChanged();
    void planChanged();

private:
    void schedulePlan();
    void startPlan();
    void finishPlan();
    void setPlanning(bool isPlanning);
    void onConfigChanged(const QString &modelPath);

    static inline PlannerRunner *s_instance = nullptr;

    ModelConfigStore &m_configs;
    SettingsStore &m_settings;
    PlanningContext m_context;
    HardwareSource m_hardware;
    QString m_modelPath;
    QVariantMap m_parameters;
    QVariantMap m_saved;
    QTimer m_debounce;
    QFutureWatcher<PlanJobResult> m_watcher;
    std::shared_ptr<std::atomic<bool>> m_cancel;
    quint64 m_generation = 0;
    quint64 m_runningGeneration = 0;
    bool m_isPlanning = false;
    bool m_isWaitingForHardware = false;
    PlanJobResult m_result;
};

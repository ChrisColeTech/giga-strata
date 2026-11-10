#include "planner_runner.h"

#include "model_config_store.h"
#include "server_build.h"
#include "settings_store.h"

#include <QJSEngine>
#include <QtConcurrent/QtConcurrentRun>

#include <utility>

namespace {

// Extra args only reach the command line; editing them never re-plans (01 section 2).
bool isPlanAffecting(const QString &key) {
    return key != QLatin1String("extra");
}

} // namespace

PlannerRunner::PlannerRunner(ModelConfigStore &configs, SettingsStore &settings, PlanningContext context,
                             HardwareSource hardware, QObject *parent)
    : QObject(parent), m_configs(configs), m_settings(settings), m_context(std::move(context)),
      m_hardware(std::move(hardware)) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(DEBOUNCE_MS);
    connect(&m_debounce, &QTimer::timeout, this, &PlannerRunner::startPlan);
    connect(&m_watcher, &QFutureWatcher<PlanJobResult>::finished, this, &PlannerRunner::finishPlan);
    connect(&m_configs, &ModelConfigStore::configChanged, this, &PlannerRunner::onConfigChanged);
}

PlannerRunner::~PlannerRunner() {
    if (m_cancel) {
        m_cancel->store(true);
    }
    m_watcher.waitForFinished();
}

void PlannerRunner::provide(PlannerRunner *instance) {
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

PlannerRunner *PlannerRunner::create(QQmlEngine *, QJSEngine *) {
    Q_ASSERT_X(s_instance, "PlannerRunner::create", "main.cpp must call PlannerRunner::provide() before loading QML");
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void PlannerRunner::setModelPath(const QString &modelPath) {
    if (modelPath == m_modelPath) {
        return;
    }
    m_modelPath = modelPath;
    m_saved = m_configs.load(modelPath);
    m_parameters = m_saved;
    emit modelPathChanged();
    emit parametersChanged();
    if (m_modelPath.isEmpty()) {
        m_debounce.stop();
        ++m_generation;
        m_result = PlanJobResult();
        emit planChanged();
        return;
    }
    schedulePlan();
}

QStringList PlannerRunner::reportLines() const {
    return m_result.run.plan ? m_result.run.plan->report : QStringList();
}

QStringList PlannerRunner::warnings() const {
    return m_result.run.plan ? m_result.run.plan->warnings : QStringList();
}

void PlannerRunner::setParameter(const QString &key, const QVariant &value) {
    if (m_parameters.value(key) == value) {
        return;
    }
    m_parameters.insert(key, value);
    emit parametersChanged();
    if (isPlanAffecting(key)) {
        schedulePlan();
    }
}

void PlannerRunner::resetToDefaults() {
    m_parameters = m_configs.defaults();
    emit parametersChanged();
    schedulePlan();
}

bool PlannerRunner::save() {
    if (m_modelPath.isEmpty() || !m_configs.save(m_modelPath, m_parameters)) {
        return false;
    }
    m_saved = m_configs.load(m_modelPath);
    m_parameters = m_saved;
    emit parametersChanged();
    return true;
}

QVariantMap PlannerRunner::runValues(const QVariantMap &parameters, const QString &modelPath,
                                     const SettingsStore &settings) {
    QVariantMap values = parameters;
    values.insert(QStringLiteral("model"), modelPath);
    values.insert(QStringLiteral("host"), settings.serveOnLan() ? QStringLiteral("0.0.0.0") : QStringLiteral("127.0.0.1"));
    values.insert(QStringLiteral("port"), settings.port());
    return values;
}

void PlannerRunner::onHardwareChanged() {
    if (m_isWaitingForHardware) {
        m_isWaitingForHardware = false;
        startPlan();
    }
}

void PlannerRunner::schedulePlan() {
    if (!m_modelPath.isEmpty()) {
        m_debounce.start();
    }
}

void PlannerRunner::startPlan() {
    if (m_modelPath.isEmpty()) {
        setPlanning(m_watcher.isRunning());
        return;
    }
    ++m_generation;
    if (m_cancel) {
        m_cancel->store(true);
    }
    if (m_watcher.isRunning()) {
        setPlanning(true);
        return;  // finishPlan() starts the latest generation once the superseded one stops
    }
    const auto hardware = m_hardware ? m_hardware() : std::nullopt;
    if (!hardware) {
        m_isWaitingForHardware = true;
        return;
    }
    PlanJobInput input = PlanJob::input(m_context, runValues(m_parameters, m_modelPath, m_settings),
                                        m_context.build->binaryFor(m_settings.serverOverride()), false);
    input.hardware = *hardware;
    m_cancel = std::make_shared<std::atomic<bool>>(false);
    input.cancel = m_cancel;
    m_runningGeneration = m_generation;
    setPlanning(true);
    m_watcher.setFuture(QtConcurrent::run([input] { return PlanJob::run(input); }));
}

void PlannerRunner::finishPlan() {
    if (m_runningGeneration != m_generation) {
        --m_generation;  // startPlan() bumps it again for the same request
        startPlan();
        if (!m_watcher.isRunning()) {
            setPlanning(false);
        }
        return;
    }
    m_result = m_watcher.result();
    setPlanning(false);
    emit planChanged();
}

void PlannerRunner::setPlanning(bool isPlanning) {
    if (m_isPlanning != isPlanning) {
        m_isPlanning = isPlanning;
        emit isPlanningChanged();
    }
}

void PlannerRunner::onConfigChanged(const QString &modelPath) {
    // An outside edit replaces the values only while nothing is being edited here.
    if (modelPath != m_modelPath || isDirty()) {
        return;
    }
    const QVariantMap saved = m_configs.load(modelPath);
    if (saved == m_saved) {
        return;
    }
    m_saved = saved;
    m_parameters = m_saved;
    emit parametersChanged();
    schedulePlan();
}

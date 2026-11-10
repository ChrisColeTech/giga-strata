#include "model_config_store.h"

#include "settings_file.h"

#include "common/model_names.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJSEngine>
#include <QJsonValue>

namespace {

const QStringList &boolKeys() {
    static const QStringList keys{QStringLiteral("mmap"),   QStringLiteral("fa"),           QStringLiteral("mtp"),
                                  QStringLiteral("vision"), QStringLiteral("record_usage"), QStringLiteral("per_expert")};
    return keys;
}

// A hand-edited number ("ctx": 32768) still reads as the string the launcher writes.
QVariant textOf(const QJsonValue &value) {
    if (value.isString()) {
        return value.toString();
    }
    return value.isDouble() ? QVariant(QString::number(value.toDouble(), 'g', 17)) : QVariant();
}

QString translated(const char *text) {
    return QCoreApplication::translate("ModelConfigStore", text);
}

} // namespace

ModelConfigStore::ModelConfigStore(const QString &directory, GpuIndices detectedGpus, QObject *parent)
    : QObject(parent), m_directory(directory), m_detectedGpus(std::move(detectedGpus)) {
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &ModelConfigStore::onFileChanged);
}

void ModelConfigStore::provide(ModelConfigStore *instance) {
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

ModelConfigStore *ModelConfigStore::create(QQmlEngine *, QJSEngine *) {
    Q_ASSERT_X(s_instance, "ModelConfigStore::create", "main.cpp must call ModelConfigStore::provide() before loading QML");
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

const QStringList &ModelConfigStore::keys() {
    static const QStringList order{QStringLiteral("gpus"),   QStringLiteral("ctx"),          QStringLiteral("margin"),
                                   QStringLiteral("ubatch"), QStringLiteral("extra"),        QStringLiteral("mmap"),
                                   QStringLiteral("fa"),     QStringLiteral("mtp"),          QStringLiteral("vision"),
                                   QStringLiteral("mmproj"), QStringLiteral("record_usage"), QStringLiteral("per_expert")};
    return order;
}

QVariantMap ModelConfigStore::load(const QString &modelPath) {
    QVariantMap values = defaults();
    if (modelPath.isEmpty()) {
        return values;
    }
    watch(modelPath);
    const auto saved = SettingsFile::read(configPath(modelPath));
    if (!saved.ok()) {
        emit errorOccurred(saved.error());
        return values;
    }
    for (const QString &key : keys()) {
        const QJsonValue value = saved.value().value(key);
        const bool isBool = boolKeys().contains(key);
        const QVariant typed = isBool ? (value.isBool() ? QVariant(value.toBool()) : QVariant()) : textOf(value);
        if (typed.isValid()) {
            values.insert(key, typed);
        }
    }
    return values;
}

bool ModelConfigStore::save(const QString &modelPath, const QVariantMap &values) {
    if (!write(modelPath, normalized(values))) {
        return false;
    }
    watch(modelPath);
    emit configChanged(modelPath);
    return true;
}

QVariantMap ModelConfigStore::defaults() const {
    QStringList gpus;
    for (const int index : m_detectedGpus ? m_detectedGpus() : QVector<int>()) {
        gpus.append(QString::number(index));
    }
    return {{QStringLiteral("gpus"), gpus.join(u',')},      {QStringLiteral("ctx"), QStringLiteral("32768")},
            {QStringLiteral("margin"), QStringLiteral("1536")}, {QStringLiteral("ubatch"), QStringLiteral("auto")},
            {QStringLiteral("extra"), QString()},            {QStringLiteral("mmap"), true},
            {QStringLiteral("fa"), true},                    {QStringLiteral("mtp"), true},
            {QStringLiteral("vision"), false},               {QStringLiteral("mmproj"), QString()},
            {QStringLiteral("record_usage"), false},         {QStringLiteral("per_expert"), false}};
}

QString ModelConfigStore::configPath(const QString &modelPath) const {
    return QDir::toNativeSeparators(QDir(m_directory).filePath(ModelNames::modelStem(modelPath) + QStringLiteral(".json")));
}

bool ModelConfigStore::ensureFile(const QString &modelPath) {
    return QFile::exists(configPath(modelPath)) || write(modelPath, defaults());
}

bool ModelConfigStore::remove(const QString &modelPath) {
    const QString path = configPath(modelPath);
    m_watcher.removePath(path);
    m_modelOfFile.remove(path);
    return !QFile::exists(path) || QFile::remove(path);
}

void ModelConfigStore::migrate(const QJsonObject &launcherSettings) {
    const QString model = launcherSettings.value(QStringLiteral("model")).toString().trimmed();
    if (model.isEmpty() || QFile::exists(configPath(model))) {
        return;
    }
    QVariantMap values = defaults();
    for (const QString &key : keys()) {
        if (launcherSettings.contains(key)) {
            values.insert(key, launcherSettings.value(key).toVariant());
        }
    }
    write(model, normalized(values));
}

QVariantMap ModelConfigStore::normalized(const QVariantMap &values) const {
    QVariantMap typed = defaults();
    for (const QString &key : keys()) {
        if (values.contains(key)) {
            const QVariant value = values.value(key);
            typed.insert(key, boolKeys().contains(key) ? QVariant(value.toBool()) : QVariant(value.toString()));
        }
    }
    return typed;
}

bool ModelConfigStore::write(const QString &modelPath, const QVariantMap &values) {
    const QString path = configPath(modelPath);
    // Keys this version does not know (a newer app, a hand edit) are kept as they are.
    const auto existing = SettingsFile::read(path);
    QJsonObject document = existing.ok() ? existing.value() : QJsonObject();
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        document.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    if (!QDir().mkpath(m_directory)) {
        emit errorOccurred(translated("Could not create the model settings folder %1.").arg(QDir::toNativeSeparators(m_directory)));
        return false;
    }
    const auto written = SettingsFile::write(path, document, keys());
    if (!written.ok()) {
        emit errorOccurred(written.error());
        return false;
    }
    return true;
}

void ModelConfigStore::watch(const QString &modelPath) {
    const QString path = configPath(modelPath);
    m_modelOfFile.insert(path, modelPath);
    if (QFile::exists(path) && !m_watcher.files().contains(path)) {
        m_watcher.addPath(path);
    }
}

void ModelConfigStore::onFileChanged(const QString &filePath) {
    const QString native = QDir::toNativeSeparators(filePath);
    const QString modelPath = m_modelOfFile.value(native);
    if (modelPath.isEmpty()) {
        return;
    }
    // An atomic replace drops the path from the watcher; watch the new file again.
    watch(modelPath);
    emit configChanged(modelPath);
}

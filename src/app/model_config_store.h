#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>
#include <QtQml/qqmlregistration.h>

#include <functional>

class QJSEngine;
class QQmlEngine;

// One JSON file of run parameters per model (05 section 2, 04 section 2.3):
// <directory>\<model stem>.json with the Python launcher's per-model keys and JSON types (02 H12:
// strings for text fields, bools for toggles). Unknown keys survive a save; writes are atomic.
// Outside edits to a file this store has read are picked up by a file watcher.
class ModelConfigStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // nvidia-smi indices of the GPUs detected now, for the default GPU list.
    using GpuIndices = std::function<QVector<int>()>;

    ModelConfigStore(const QString &directory, GpuIndices detectedGpus, QObject *parent = nullptr);

    static void provide(ModelConfigStore *instance);
    static ModelConfigStore *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // The per-model keys, in the order files are written.
    static const QStringList &keys();

    Q_INVOKABLE QVariantMap load(const QString &modelPath);
    Q_INVOKABLE bool save(const QString &modelPath, const QVariantMap &values);
    Q_INVOKABLE QVariantMap defaults() const;
    Q_INVOKABLE QString configPath(const QString &modelPath) const;

    // Writes the defaults when the model has no file yet (Edit config.json). False on failure.
    bool ensureFile(const QString &modelPath);
    // Deletes the model's file if it has one. False when it exists and cannot be removed.
    bool remove(const QString &modelPath);
    // First start: the model in launcher_settings.json gets a file seeded from that file's
    // values, unless it already has one.
    void migrate(const QJsonObject &launcherSettings);

signals:
    void configChanged(const QString &modelPath);
    void errorOccurred(const QString &message);

private:
    QVariantMap normalized(const QVariantMap &values) const;
    bool write(const QString &modelPath, const QVariantMap &values);
    void watch(const QString &modelPath);
    void onFileChanged(const QString &filePath);

    static inline ModelConfigStore *s_instance = nullptr;

    QString m_directory;
    GpuIndices m_detectedGpus;
    QFileSystemWatcher m_watcher;
    QHash<QString, QString> m_modelOfFile;  // config file -> model path, for the watcher
};

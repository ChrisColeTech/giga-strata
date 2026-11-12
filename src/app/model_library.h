#pragma once

#include "model_entry.h"
#include "model_table_model.h"
#include "model_table_proxy.h"
#include "shell_actions.h"

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>
#include <QtQml/qqmlregistration.h>

class ModelConfigStore;
class QJSEngine;
class QQmlEngine;
class SettingsStore;

// The Models table (05 section 1, 04 section 2): the models found in the folders saved in
// SettingsStore, scanned on a worker, sorted and filtered by the header, plus the row actions
// (reveal, edit config, delete). A rescan requested during a scan runs right after it.
class ModelLibrary : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QAbstractItemModel *tableModel READ tableModel CONSTANT)
    // Every model, unsorted and unfiltered: the Serving picker must not lose models to a filter
    // set on the Models screen.
    Q_PROPERTY(QAbstractItemModel *allModels READ allModels CONSTANT)
    Q_PROPERTY(QStringList folders READ folders NOTIFY foldersChanged)
    Q_PROPERTY(bool isScanning READ isScanning NOTIFY isScanningChanged)
    Q_PROPERTY(int modelCount READ modelCount NOTIFY modelCountChanged)
    Q_PROPERTY(QStringList activeFilters READ activeFilters NOTIFY activeFiltersChanged)

public:
    ModelLibrary(SettingsStore &settings, ModelConfigStore &configs, QString usageDirectory, ShellActions shell,
                 QObject *parent = nullptr);
    ~ModelLibrary() override;

    static void provide(ModelLibrary *instance);
    static ModelLibrary *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    QAbstractItemModel *tableModel() { return &m_proxy; }
    QAbstractItemModel *allModels() { return &m_table; }
    QStringList folders() const;
    bool isScanning() const { return m_watcher.isRunning(); }
    int modelCount() const { return m_table.rowCount(); }
    QStringList activeFilters() const;
    const QVector<ModelEntry> &entries() const { return m_table.entries(); }

    Q_INVOKABLE void addFolder(const QUrl &folder);
    Q_INVOKABLE void removeFolder(const QString &folder);
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void sortBy(const QString &role, bool isAscending);
    Q_INVOKABLE QStringList filterValues(const QString &role) const;
    Q_INVOKABLE void setFilter(const QString &role, const QVariant &value);
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE void revealInExplorer(const QString &path);
    Q_INVOKABLE void openConfigFile(const QString &path);
    // The name API clients send as "model": the server runs with --alias <model stem>.
    Q_INVOKABLE QString identifier(const QString &path) const;
    Q_INVOKABLE void copyIdentifier(const QString &path);
    Q_INVOKABLE void deleteModel(const QString &path);

signals:
    void foldersChanged();
    void isScanningChanged();
    void modelCountChanged();
    void activeFiltersChanged();
    void modelDeleted(const QString &path);
    void errorOccurred(const QString &message);

private:
    void onSettingChanged(const QString &key);
    void applyScan();

    static inline ModelLibrary *s_instance = nullptr;

    SettingsStore &m_settings;
    ModelConfigStore &m_configs;
    QString m_usageDirectory;
    ShellActions m_shell;
    ModelTableModel m_table;
    ModelTableProxy m_proxy;
    QFutureWatcher<QVector<ModelEntry>> m_watcher;
    bool m_isRescanPending = false;
};

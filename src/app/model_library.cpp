#include "model_library.h"

#include "model_config_store.h"
#include "model_scanner.h"
#include "settings_store.h"

#include "common/model_names.h"
#include "gguf/local_model_source.h"
#include "gguf/shard_cache.h"
#include "gguf/shard_finder.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJSEngine>
#include <QJSValue>
#include <QtConcurrent/QtConcurrentRun>

#include <utility>

namespace {

// One scan reads every header in the folders; a scan-local cache lets the draft and projector
// searches reuse them, and is dropped with the scan so idle memory stays small.
constexpr int SCAN_CACHE_CAPACITY = 64;

QString translated(const char *text) {
    return QCoreApplication::translate("ModelLibrary", text);
}

} // namespace

ModelLibrary::ModelLibrary(SettingsStore &settings, ModelConfigStore &configs, QString usageDirectory,
                           ShellActions shell, QObject *parent)
    : QObject(parent), m_settings(settings), m_configs(configs), m_usageDirectory(std::move(usageDirectory)),
      m_shell(std::move(shell)) {
    m_proxy.setSourceModel(&m_table);
    m_proxy.setSortRole(ModelTableModel::NameRole);
    m_proxy.sort(0, Qt::AscendingOrder);
    connect(&m_watcher, &QFutureWatcher<QVector<ModelEntry>>::finished, this, &ModelLibrary::applyScan);
    connect(&m_settings, &SettingsStore::changed, this, &ModelLibrary::onSettingChanged);
    rescan();
}

ModelLibrary::~ModelLibrary() {
    m_watcher.waitForFinished();
}

void ModelLibrary::provide(ModelLibrary *instance) {
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

ModelLibrary *ModelLibrary::create(QQmlEngine *, QJSEngine *) {
    Q_ASSERT_X(s_instance, "ModelLibrary::create", "main.cpp must call ModelLibrary::provide() before loading QML");
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

QStringList ModelLibrary::folders() const {
    return m_settings.modelFolders();
}

QStringList ModelLibrary::activeFilters() const {
    QStringList roles;
    const QHash<int, QByteArray> names = m_table.roleNames();
    for (const int role : m_proxy.filteredRoles()) {
        roles.append(QString::fromUtf8(names.value(role)));
    }
    roles.sort();
    return roles;
}

void ModelLibrary::addFolder(const QUrl &folder) {
    const QString path = QDir::toNativeSeparators(QDir::cleanPath(folder.toLocalFile()));
    QStringList current = folders();
    if (path.isEmpty() || current.contains(path, Qt::CaseInsensitive)) {
        return;
    }
    current.append(path);
    m_settings.setModelFolders(current);
}

void ModelLibrary::removeFolder(const QString &folder) {
    QStringList current = folders();
    current.removeIf([&folder](const QString &path) { return path.compare(folder, Qt::CaseInsensitive) == 0; });
    m_settings.setModelFolders(current);
}

void ModelLibrary::rescan() {
    if (m_watcher.isRunning()) {
        m_isRescanPending = true;
        return;
    }
    const QStringList scanned = folders();
    const QString usageDirectory = m_usageDirectory;
    m_watcher.setFuture(QtConcurrent::run([scanned, usageDirectory] {
        ShardCache cache(SCAN_CACHE_CAPACITY);
        return ModelScanner::scan(LocalModelSource(&cache), scanned, usageDirectory);
    }));
    emit isScanningChanged();
}

void ModelLibrary::sortBy(const QString &role, bool isAscending) {
    const int number = m_table.roleFor(role);
    if (number >= 0) {
        m_proxy.setSortRole(number);
        m_proxy.sort(0, isAscending ? Qt::AscendingOrder : Qt::DescendingOrder);
    }
}

QStringList ModelLibrary::filterValues(const QString &role) const {
    const int number = m_table.roleFor(role);
    QStringList values;
    for (int row = 0; number >= 0 && row < m_table.rowCount(); ++row) {
        const QString value = m_table.index(row).data(number).toString();
        if (!values.contains(value)) {
            values.append(value);
        }
    }
    values.sort(Qt::CaseInsensitive);
    return values;
}

void ModelLibrary::setFilter(const QString &role, const QVariant &value) {
    const int number = m_table.roleFor(role);
    if (number >= 0) {
        // A JS array from QML arrives wrapped as a QJSValue; the proxy needs the plain list.
        const QVariant plain = value.typeId() == qMetaTypeId<QJSValue>() ? value.value<QJSValue>().toVariant() : value;
        m_proxy.setRoleFilter(number, plain);
        emit activeFiltersChanged();
    }
}

void ModelLibrary::clearFilters() {
    m_proxy.clearRoleFilters();
    emit activeFiltersChanged();
}

void ModelLibrary::revealInExplorer(const QString &path) {
    const QString native = QDir::toNativeSeparators(path);
    if (!m_shell.startDetached(QStringLiteral("explorer.exe"), QStringLiteral("/select,\"%1\"").arg(native))) {
        emit errorOccurred(translated("Could not open File Explorer at %1.").arg(native));
    }
}

QString ModelLibrary::identifier(const QString &path) const {
    return ModelNames::modelStem(path);
}

void ModelLibrary::copyIdentifier(const QString &path) {
    QGuiApplication::clipboard()->setText(identifier(path));
}

void ModelLibrary::openConfigFile(const QString &path) {
    if (!m_configs.ensureFile(path)) {
        return;  // the store already said why
    }
    const QString config = m_configs.configPath(path);
    if (!m_shell.openUrl(QUrl::fromLocalFile(config))) {
        emit errorOccurred(translated("Could not open %1. Set a default program for .json files.").arg(config));
    }
}

void ModelLibrary::deleteModel(const QString &path) {
    const auto shards = ShardFinder::shardPaths(LocalModelSource(), path);
    const QStringList files = shards.ok() ? shards.value() : QStringList{path};
    for (const QString &file : files) {
        if (QFile::exists(file) && !QFile::remove(file)) {
            emit errorOccurred(translated("Could not delete %1. Stop the server if it is using this model, then "
                                          "try again.")
                                   .arg(QDir::toNativeSeparators(file)));
            return;
        }
    }
    if (!m_configs.remove(path)) {
        emit errorOccurred(translated("The model files were deleted, but its settings file %1 could not be.")
                               .arg(m_configs.configPath(path)));
    }
    if (m_table.removePath(path)) {
        emit modelCountChanged();
    }
    emit modelDeleted(path);
}

void ModelLibrary::onSettingChanged(const QString &key) {
    if (key == SettingsStore::KEY_MODEL_FOLDERS) {
        emit foldersChanged();
        rescan();
    }
}

void ModelLibrary::applyScan() {
    m_table.setEntries(m_watcher.result());
    emit isScanningChanged();
    emit modelCountChanged();
    if (m_isRescanPending) {
        m_isRescanPending = false;
        rescan();
    }
}

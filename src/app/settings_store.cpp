#include "settings_store.h"

#include "settings_file.h"

#include <QDir>
#include <QFileInfo>
#include <QJSEngine>
#include <QJsonArray>

#include <algorithm>
#include <optional>

namespace {

// Defaults of the Tk launcher's StringVar/BooleanVar, in its setting_vars() order. The binary
// default is empty because resolving the shipped build belongs to the server finder.
const QJsonObject &configurationDefaults() {
    static const QJsonObject defaults{
        {SettingsStore::KEY_BINARY, QString()},
        {SettingsStore::KEY_MODEL, QString()},
        {SettingsStore::KEY_GPUS, QStringLiteral("0,1")},
        {SettingsStore::KEY_CONTEXT, QStringLiteral("32768")},
        {SettingsStore::KEY_MARGIN, QStringLiteral("1536")},
        {SettingsStore::KEY_UBATCH, QStringLiteral("auto")},
        {SettingsStore::KEY_HOST, QStringLiteral("0.0.0.0")},
        {SettingsStore::KEY_PORT, QStringLiteral("8080")},
        {SettingsStore::KEY_EXTRA, QString()},
        {SettingsStore::KEY_MMAP, true},
        {SettingsStore::KEY_FLASH_ATTENTION, true},
        {SettingsStore::KEY_AUTO_SERVER, true},
        {SettingsStore::KEY_MTP, true},
        {SettingsStore::KEY_VISION, false},
        {SettingsStore::KEY_MMPROJ, QString()},
        {SettingsStore::KEY_RECORD_USAGE, false},
        {SettingsStore::KEY_PER_EXPERT, false},
    };
    return defaults;
}

const QStringList &fileKeyOrder() {
    static const QStringList order{
        SettingsStore::KEY_BINARY,      SettingsStore::KEY_MODEL,        SettingsStore::KEY_GPUS,
        SettingsStore::KEY_CONTEXT,     SettingsStore::KEY_MARGIN,       SettingsStore::KEY_UBATCH,
        SettingsStore::KEY_HOST,        SettingsStore::KEY_PORT,         SettingsStore::KEY_EXTRA,
        SettingsStore::KEY_MMAP,        SettingsStore::KEY_FLASH_ATTENTION, SettingsStore::KEY_AUTO_SERVER,
        SettingsStore::KEY_MTP,         SettingsStore::KEY_VISION,       SettingsStore::KEY_MMPROJ,
        SettingsStore::KEY_RECORD_USAGE, SettingsStore::KEY_PER_EXPERT, SettingsStore::KEY_SERVER_CACHE,
        SettingsStore::KEY_SERVER_CANDIDATES, SettingsStore::KEY_SPLIT_RATIO, SettingsStore::KEY_MODEL_FOLDERS,
        SettingsStore::KEY_SERVE_ON_LAN, SettingsStore::KEY_SERVER_OVERRIDE, SettingsStore::KEY_REDUCE_MOTION,
        SettingsStore::KEY_KEEP_SERVING_IN_TRAY, SettingsStore::KEY_MINIMIZE_TO_TRAY,
        SettingsStore::KEY_LOG_DRAWER_HEIGHT, SettingsStore::KEY_PARAMETERS_PANEL_WIDTH, SettingsStore::KEY_IS_LOG_OPEN,
        SettingsStore::KEY_IS_PARAMETERS_OPEN, SettingsStore::KEY_GPU_HEADROOM, SettingsStore::KEY_MODEL_COLUMN_WIDTHS,
    };
    return order;
}

constexpr double DEFAULT_SPLIT_RATIO = 0.45;
constexpr double MIN_SPLIT_RATIO = 0.1;
constexpr double MAX_SPLIT_RATIO = 0.9;

// A Tk StringVar accepted numbers too; a hand-edited "ctx": 32768 must not be lost.
std::optional<QJsonValue> coerced(const QJsonValue &saved, const QJsonValue &fallback) {
    if (fallback.isBool()) {
        return saved.isBool() ? std::optional<QJsonValue>(saved) : std::nullopt;
    }
    if (saved.isString()) {
        return saved;
    }
    if (saved.isDouble()) {
        return QJsonValue(QString::number(saved.toDouble(), 'g', 17));
    }
    return std::nullopt;
}

// llama_launcher.read_settings: spaces removed, empty items skipped, every item an integer.
std::optional<QList<int>> parsedGpuList(const QString &gpuList) {
    QList<int> indices;
    const QStringList items = QString(gpuList).remove(QLatin1Char(' ')).split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &item : items) {
        bool isNumber = false;
        const int index = item.toInt(&isNumber);
        if (!isNumber) {
            return std::nullopt;
        }
        indices.append(index);
    }
    return indices;
}

} // namespace

SettingsStore::SettingsStore(const QString &filePath, QObject *parent) : QObject(parent), m_filePath(filePath) {
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(SAVE_DEBOUNCE_MS);
    connect(&m_saveTimer, &QTimer::timeout, this, &SettingsStore::flush);
    load();
}

SettingsStore::~SettingsStore() {
    flush();
}

void SettingsStore::provide(SettingsStore *instance) {
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

SettingsStore *SettingsStore::create(QQmlEngine *, QJSEngine *) {
    Q_ASSERT_X(s_instance, "SettingsStore::create", "main.cpp must call SettingsStore::provide() before loading QML");
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void SettingsStore::load() {
    const Result<QJsonObject> saved = SettingsFile::read(m_filePath);
    if (!saved.ok()) {
        m_loadError = saved.error();
    }
    m_document = saved.ok() ? saved.value() : QJsonObject();
    const QJsonObject &defaults = configurationDefaults();
    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
        m_document.insert(it.key(), coerced(m_document.value(it.key()), it.value()).value_or(it.value()));
    }
    m_savedWindowValues = windowValues();
    m_savedEffectiveValues = effectiveWindowValues();
}

const QStringList &SettingsStore::windowKeys() {
    static const QStringList keys{KEY_MODEL_FOLDERS,   KEY_REDUCE_MOTION,    KEY_KEEP_SERVING_IN_TRAY,
                                  KEY_MINIMIZE_TO_TRAY, KEY_SERVER_OVERRIDE, KEY_GPU_HEADROOM};
    return keys;
}

QJsonObject SettingsStore::windowValues() const {
    QJsonObject values;
    for (const QString &key : windowKeys()) {
        if (m_document.contains(key)) {
            values.insert(key, m_document.value(key));
        }
    }
    return values;
}

QJsonObject SettingsStore::effectiveWindowValues() const {
    return {{KEY_MODEL_FOLDERS, QJsonArray::fromStringList(modelFolders())},
            {KEY_REDUCE_MOTION, reduceMotion()},
            {KEY_KEEP_SERVING_IN_TRAY, keepServingInTray()},
            {KEY_MINIMIZE_TO_TRAY, minimizeToTray()},
            {KEY_SERVER_OVERRIDE, serverOverride()},
            {KEY_GPU_HEADROOM, gpuHeadroom()}};
}

void SettingsStore::saveSettings() {
    if (!hasUnsavedSettings()) {
        return;
    }
    m_savedWindowValues = windowValues();
    m_savedEffectiveValues = effectiveWindowValues();
    m_isDirty = true;
    flush();
    emit valuesChanged();
}

void SettingsStore::discardSettings() {
    if (!hasUnsavedSettings()) {
        return;
    }
    QStringList restored;
    for (const QString &key : windowKeys()) {
        const QJsonValue saved = m_savedWindowValues.value(key);
        if (m_document.value(key) == saved) {
            continue;
        }
        if (saved.isUndefined()) {
            m_document.remove(key);
        } else {
            m_document.insert(key, saved);
        }
        restored.append(key);
    }
    for (const QString &key : restored) {
        emit changed(key);
    }
    emit valuesChanged();
}

void SettingsStore::store(QLatin1StringView key, const QJsonValue &value) {
    if (m_document.value(key) == value) {
        return;
    }
    m_document.insert(key, value);
    m_isDirty = true;
    m_saveTimer.start();
    emit changed(QString(key));
    emit valuesChanged();
}

void SettingsStore::flush() {
    m_saveTimer.stop();
    if (!m_isDirty) {
        return;
    }
    // Unsaved Settings-window edits stay out of the file.
    QJsonObject document = m_document;
    for (const QString &key : windowKeys()) {
        const QJsonValue saved = m_savedWindowValues.value(key);
        if (saved.isUndefined()) {
            document.remove(key);
        } else {
            document.insert(key, saved);
        }
    }
    const Result<void> written = SettingsFile::write(m_filePath, document, fileKeyOrder());
    if (!written.ok()) {
        emit errorOccurred(written.error());
        return;
    }
    m_isDirty = false;
}

double SettingsStore::uiSplitRatio() const {
    const double ratio = m_document.value(KEY_SPLIT_RATIO).toDouble(DEFAULT_SPLIT_RATIO);
    return std::clamp(ratio, MIN_SPLIT_RATIO, MAX_SPLIT_RATIO);
}

void SettingsStore::setUiSplitRatio(double value) {
    store(KEY_SPLIT_RATIO, std::clamp(value, MIN_SPLIT_RATIO, MAX_SPLIT_RATIO));
}

QStringList SettingsStore::modelFolders() const {
    QStringList folders;
    for (const QJsonValue &folder : m_document.value(KEY_MODEL_FOLDERS).toArray()) {
        if (folder.isString() && !folder.toString().isEmpty()) {
            folders.append(folder.toString());
        }
    }
    return folders;
}

void SettingsStore::setModelFolders(const QStringList &folders) {
    store(KEY_MODEL_FOLDERS, QJsonArray::fromStringList(folders));
}

bool SettingsStore::serveOnLan() const {
    // Before the key existed the Python launcher's host decided it; its default was 0.0.0.0.
    const QJsonValue saved = m_document.value(KEY_SERVE_ON_LAN);
    return saved.isBool() ? saved.toBool() : host() == QLatin1String("0.0.0.0");
}

void SettingsStore::setServeOnLan(bool isOn) {
    store(KEY_HOST, isOn ? QStringLiteral("0.0.0.0") : QStringLiteral("127.0.0.1"));
    store(KEY_SERVE_ON_LAN, isOn);
}

QString SettingsStore::reduceMotion() const {
    return text(KEY_REDUCE_MOTION) == QLatin1String("on") ? QStringLiteral("on") : QStringLiteral("system");
}

void SettingsStore::setReduceMotion(const QString &mode) {
    store(KEY_REDUCE_MOTION, mode == QLatin1String("on") ? QStringLiteral("on") : QStringLiteral("system"));
}

QJsonObject SettingsStore::configuration() const {
    QJsonObject result;
    for (const QString &key : configurationDefaults().keys()) {
        result.insert(key, m_document.value(key));
    }
    return result;
}

void SettingsStore::applyConfiguration(const QJsonObject &configuration) {
    const QJsonObject &defaults = configurationDefaults();
    for (auto it = configuration.begin(); it != configuration.end(); ++it) {
        if (!defaults.contains(it.key())) {
            continue;
        }
        const std::optional<QJsonValue> value = coerced(it.value(), defaults.value(it.key()));
        if (value) {
            store(QLatin1StringView(it.key().toLatin1()), *value);
        }
    }
}

int SettingsStore::gpuOrder(int gpuIndex) const {
    const std::optional<QList<int>> selected = parsedGpuList(gpus());
    return selected ? static_cast<int>(selected->indexOf(gpuIndex)) : -1;
}

void SettingsStore::toggleGpu(int gpuIndex, const QVariantList &chipOrder) {
    QList<int> selected = parsedGpuList(gpus()).value_or(QList<int>());
    if (!selected.removeOne(gpuIndex)) {
        selected.append(gpuIndex);
    }
    const auto position = [&chipOrder](int index) {
        const qsizetype found = chipOrder.indexOf(QVariant(index));
        return found < 0 ? chipOrder.size() : found;
    };
    std::stable_sort(selected.begin(), selected.end(), [&](int a, int b) { return position(a) < position(b); });
    QStringList items;
    for (const int index : selected) {
        items.append(QString::number(index));
    }
    setGpus(items.join(QLatin1Char(',')));
}

bool SettingsStore::isGpuListValid(const QString &gpuList) const {
    const std::optional<QList<int>> parsed = parsedGpuList(gpuList);
    return parsed && !parsed->isEmpty();
}

QString SettingsStore::nativePath(const QUrl &url) const {
    return QDir::toNativeSeparators(QDir::cleanPath(url.toLocalFile()));
}

QUrl SettingsStore::folderUrl(const QString &filePath) const {
    const QFileInfo file(filePath.trimmed());
    if (filePath.trimmed().isEmpty() || !file.dir().exists()) {
        return QUrl();
    }
    return QUrl::fromLocalFile(file.absolutePath());
}

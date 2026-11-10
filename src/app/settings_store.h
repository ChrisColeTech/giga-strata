#pragma once

#include <QJsonObject>
#include <QLatin1StringView>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QQmlEngine;

// The only reader and writer of launcher_settings.json (R22). Superset-compatible with the Python
// launcher (02 H12): text fields stay JSON strings, toggles JSON bools, unknown keys and the
// server cache pass through verbatim. Every edit emits changed(key) and schedules a debounced
// atomic write; flush() writes immediately and runs again on destruction.
class SettingsStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString binary READ binary WRITE setBinary NOTIFY valuesChanged)
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY valuesChanged)
    Q_PROPERTY(QString gpus READ gpus WRITE setGpus NOTIFY valuesChanged)
    Q_PROPERTY(QString contextSize READ contextSize WRITE setContextSize NOTIFY valuesChanged)
    Q_PROPERTY(QString margin READ margin WRITE setMargin NOTIFY valuesChanged)
    Q_PROPERTY(QString ubatch READ ubatch WRITE setUbatch NOTIFY valuesChanged)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY valuesChanged)
    Q_PROPERTY(QString port READ port WRITE setPort NOTIFY valuesChanged)
    Q_PROPERTY(QString extraArgs READ extraArgs WRITE setExtraArgs NOTIFY valuesChanged)
    Q_PROPERTY(QString mmproj READ mmproj WRITE setMmproj NOTIFY valuesChanged)
    Q_PROPERTY(bool autoServer READ autoServer WRITE setAutoServer NOTIFY valuesChanged)
    Q_PROPERTY(bool useMmap READ useMmap WRITE setUseMmap NOTIFY valuesChanged)
    Q_PROPERTY(bool useFlashAttention READ useFlashAttention WRITE setUseFlashAttention NOTIFY valuesChanged)
    Q_PROPERTY(bool useMtp READ useMtp WRITE setUseMtp NOTIFY valuesChanged)
    Q_PROPERTY(bool useVision READ useVision WRITE setUseVision NOTIFY valuesChanged)
    Q_PROPERTY(bool useUsageRecording READ useUsageRecording WRITE setUseUsageRecording NOTIFY valuesChanged)
    Q_PROPERTY(bool usePerExpert READ usePerExpert WRITE setUsePerExpert NOTIFY valuesChanged)
    Q_PROPERTY(double uiSplitRatio READ uiSplitRatio WRITE setUiSplitRatio NOTIFY valuesChanged)
    Q_PROPERTY(QString loadError READ loadError CONSTANT)
    // App-wide keys (05 section 6); per-model parameters live in ModelConfigStore.
    Q_PROPERTY(QStringList modelFolders READ modelFolders WRITE setModelFolders NOTIFY valuesChanged)
    Q_PROPERTY(bool serveOnLan READ serveOnLan WRITE setServeOnLan NOTIFY valuesChanged)
    Q_PROPERTY(QString serverOverride READ serverOverride WRITE setServerOverride NOTIFY valuesChanged)
    Q_PROPERTY(QString reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY valuesChanged)
    Q_PROPERTY(bool keepServingInTray READ keepServingInTray WRITE setKeepServingInTray NOTIFY valuesChanged)
    Q_PROPERTY(bool minimizeToTray READ minimizeToTray WRITE setMinimizeToTray NOTIFY valuesChanged)
    Q_PROPERTY(double logDrawerHeight READ logDrawerHeight WRITE setLogDrawerHeight NOTIFY valuesChanged)
    Q_PROPERTY(double parametersPanelWidth READ parametersPanelWidth WRITE setParametersPanelWidth NOTIFY valuesChanged)
    Q_PROPERTY(bool isLogOpen READ isLogOpen WRITE setLogOpen NOTIFY valuesChanged)
    Q_PROPERTY(bool isParametersOpen READ isParametersOpen WRITE setParametersOpen NOTIFY valuesChanged)

public:
    // JSON keys, identical to llama_launcher.setting_vars() plus the keys the C++ app appends.
    static constexpr QLatin1StringView KEY_BINARY{"binary"};
    static constexpr QLatin1StringView KEY_MODEL{"model"};
    static constexpr QLatin1StringView KEY_GPUS{"gpus"};
    static constexpr QLatin1StringView KEY_CONTEXT{"ctx"};
    static constexpr QLatin1StringView KEY_MARGIN{"margin"};
    static constexpr QLatin1StringView KEY_UBATCH{"ubatch"};
    static constexpr QLatin1StringView KEY_HOST{"host"};
    static constexpr QLatin1StringView KEY_PORT{"port"};
    static constexpr QLatin1StringView KEY_EXTRA{"extra"};
    static constexpr QLatin1StringView KEY_MMAP{"mmap"};
    static constexpr QLatin1StringView KEY_FLASH_ATTENTION{"fa"};
    static constexpr QLatin1StringView KEY_AUTO_SERVER{"auto_server"};
    static constexpr QLatin1StringView KEY_MTP{"mtp"};
    static constexpr QLatin1StringView KEY_VISION{"vision"};
    static constexpr QLatin1StringView KEY_MMPROJ{"mmproj"};
    static constexpr QLatin1StringView KEY_RECORD_USAGE{"record_usage"};
    static constexpr QLatin1StringView KEY_PER_EXPERT{"per_expert"};
    static constexpr QLatin1StringView KEY_SERVER_CACHE{"server_cache"};
    static constexpr QLatin1StringView KEY_SERVER_CANDIDATES{"server_candidates"};
    static constexpr QLatin1StringView KEY_SPLIT_RATIO{"uiSplitRatio"};
    static constexpr QLatin1StringView KEY_MODEL_FOLDERS{"model_folders"};
    static constexpr QLatin1StringView KEY_SERVE_ON_LAN{"serve_on_lan"};
    static constexpr QLatin1StringView KEY_SERVER_OVERRIDE{"server_override"};
    static constexpr QLatin1StringView KEY_REDUCE_MOTION{"reduce_motion"};
    static constexpr QLatin1StringView KEY_KEEP_SERVING_IN_TRAY{"keep_serving_in_tray"};
    static constexpr QLatin1StringView KEY_MINIMIZE_TO_TRAY{"minimize_to_tray"};
    static constexpr QLatin1StringView KEY_LOG_DRAWER_HEIGHT{"log_drawer_height"};
    static constexpr QLatin1StringView KEY_PARAMETERS_PANEL_WIDTH{"parameters_panel_width"};
    static constexpr QLatin1StringView KEY_IS_LOG_OPEN{"is_log_open"};
    static constexpr QLatin1StringView KEY_IS_PARAMETERS_OPEN{"is_parameters_open"};
    static constexpr double DEFAULT_LOG_DRAWER_HEIGHT = 0;  // 0: the log fills the Serving screen's remaining height
    static constexpr double DEFAULT_PARAMETERS_PANEL_WIDTH = 420;
    static constexpr int SAVE_DEBOUNCE_MS = 800;

    explicit SettingsStore(const QString &filePath, QObject *parent = nullptr);
    ~SettingsStore() override;

    static void provide(SettingsStore *instance);
    static SettingsStore *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    QString binary() const { return text(KEY_BINARY); }
    QString model() const { return text(KEY_MODEL); }
    QString gpus() const { return text(KEY_GPUS); }
    QString contextSize() const { return text(KEY_CONTEXT); }
    QString margin() const { return text(KEY_MARGIN); }
    QString ubatch() const { return text(KEY_UBATCH); }
    QString host() const { return text(KEY_HOST); }
    QString port() const { return text(KEY_PORT); }
    QString extraArgs() const { return text(KEY_EXTRA); }
    QString mmproj() const { return text(KEY_MMPROJ); }
    bool autoServer() const { return flag(KEY_AUTO_SERVER); }
    bool useMmap() const { return flag(KEY_MMAP); }
    bool useFlashAttention() const { return flag(KEY_FLASH_ATTENTION); }
    bool useMtp() const { return flag(KEY_MTP); }
    bool useVision() const { return flag(KEY_VISION); }
    bool useUsageRecording() const { return flag(KEY_RECORD_USAGE); }
    bool usePerExpert() const { return flag(KEY_PER_EXPERT); }
    double uiSplitRatio() const;
    const QString &loadError() const { return m_loadError; }
    QStringList modelFolders() const;
    bool serveOnLan() const;
    QString serverOverride() const { return text(KEY_SERVER_OVERRIDE); }
    QString reduceMotion() const;
    bool keepServingInTray() const { return m_document.value(KEY_KEEP_SERVING_IN_TRAY).toBool(true); }
    bool minimizeToTray() const { return flag(KEY_MINIMIZE_TO_TRAY); }
    double logDrawerHeight() const { return m_document.value(KEY_LOG_DRAWER_HEIGHT).toDouble(DEFAULT_LOG_DRAWER_HEIGHT); }
    double parametersPanelWidth() const {
        return m_document.value(KEY_PARAMETERS_PANEL_WIDTH).toDouble(DEFAULT_PARAMETERS_PANEL_WIDTH);
    }
    bool isLogOpen() const { return flag(KEY_IS_LOG_OPEN); }
    bool isParametersOpen() const { return flag(KEY_IS_PARAMETERS_OPEN); }

    void setBinary(const QString &value) { store(KEY_BINARY, value); }
    void setModel(const QString &value) { store(KEY_MODEL, value); }
    void setGpus(const QString &value) { store(KEY_GPUS, value); }
    void setContextSize(const QString &value) { store(KEY_CONTEXT, value); }
    void setMargin(const QString &value) { store(KEY_MARGIN, value); }
    void setUbatch(const QString &value) { store(KEY_UBATCH, value); }
    void setHost(const QString &value) { store(KEY_HOST, value); }
    void setPort(const QString &value) { store(KEY_PORT, value); }
    void setExtraArgs(const QString &value) { store(KEY_EXTRA, value); }
    void setMmproj(const QString &value) { store(KEY_MMPROJ, value); }
    void setAutoServer(bool value) { store(KEY_AUTO_SERVER, value); }
    void setUseMmap(bool value) { store(KEY_MMAP, value); }
    void setUseFlashAttention(bool value) { store(KEY_FLASH_ATTENTION, value); }
    void setUseMtp(bool value) { store(KEY_MTP, value); }
    void setUseVision(bool value) { store(KEY_VISION, value); }
    void setUseUsageRecording(bool value) { store(KEY_RECORD_USAGE, value); }
    void setUsePerExpert(bool value) { store(KEY_PER_EXPERT, value); }
    void setUiSplitRatio(double value);
    void setModelFolders(const QStringList &folders);
    // Also sets host: 0.0.0.0 on the LAN, 127.0.0.1 for this PC only (05 section 6).
    void setServeOnLan(bool isOn);
    void setServerOverride(const QString &path) { store(KEY_SERVER_OVERRIDE, path); }
    void setReduceMotion(const QString &mode);
    void setKeepServingInTray(bool isOn) { store(KEY_KEEP_SERVING_IN_TRAY, isOn); }
    void setMinimizeToTray(bool isOn) { store(KEY_MINIMIZE_TO_TRAY, isOn); }
    void setLogDrawerHeight(double height) { store(KEY_LOG_DRAWER_HEIGHT, height); }
    void setParametersPanelWidth(double width) { store(KEY_PARAMETERS_PANEL_WIDTH, width); }
    void setLogOpen(bool isOpen) { store(KEY_IS_LOG_OPEN, isOpen); }
    void setParametersOpen(bool isOpen) { store(KEY_IS_PARAMETERS_OPEN, isOpen); }

    // Every user-facing setting (the Python launcher's setting_vars), without caches or UI state.
    QJsonObject configuration() const;
    void applyConfiguration(const QJsonObject &configuration);
    QJsonObject document() const { return m_document; }

    // GPU chips (S3): position of an nvidia-smi index in the CUDA order, -1 when not selected.
    Q_INVOKABLE int gpuOrder(int gpuIndex) const;
    // Includes or excludes one GPU; the result follows `chipOrder` (nvidia-smi indices).
    Q_INVOKABLE void toggleGpu(int gpuIndex, const QVariantList &chipOrder);
    Q_INVOKABLE bool isGpuListValid(const QString &gpuList) const;
    // File pickers hand back URLs; the file keeps native paths like os.path.normpath did.
    Q_INVOKABLE QString nativePath(const QUrl &url) const;
    Q_INVOKABLE QUrl folderUrl(const QString &filePath) const;

public slots:
    void flush();

signals:
    void valuesChanged();
    void changed(const QString &key);
    void errorOccurred(const QString &message);

private:
    QString text(QLatin1StringView key) const { return m_document.value(key).toString(); }
    bool flag(QLatin1StringView key) const { return m_document.value(key).toBool(); }
    void store(QLatin1StringView key, const QJsonValue &value);
    void load();

    static inline SettingsStore *s_instance = nullptr;

    QString m_filePath;
    QJsonObject m_document;
    QString m_loadError;
    QTimer m_saveTimer;
    bool m_isDirty = false;
};

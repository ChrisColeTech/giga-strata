#pragma once

#include <QHash>
#include <QMutex>
#include <QProcessEnvironment>
#include <QSet>
#include <QString>
#include <QStringList>

class ServerEnvironment;

// llama_launcher.detect_flags: which optional flags a llama-server build understands, scraped
// from its --help. Cached per binary path string inside this object (the Python cache is a
// module global); safe to call from worker threads.
class FlagProbe {
public:
    explicit FlagProbe(const ServerEnvironment &environment);

    QSet<QString> flags(const QString &binary, const QProcessEnvironment &environment);

    // The scan alone: every known flag that occurs anywhere in the help text.
    static QSet<QString> scan(const QString &helpText);

    // The ten strings detect_flags looks for, in its order.
    static QStringList knownFlags();

private:
    const ServerEnvironment &m_environment;
    QMutex m_mutex;
    QHash<QString, QSet<QString>> m_cache;
};

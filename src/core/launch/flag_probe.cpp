#include "launch/flag_probe.h"

#include "launch/server_environment.h"

#include <QMutexLocker>

FlagProbe::FlagProbe(const ServerEnvironment &environment) : m_environment(environment) {}

QSet<QString> FlagProbe::flags(const QString &binary, const QProcessEnvironment &environment) {
    {
        const QMutexLocker locker(&m_mutex);
        const auto cached = m_cache.constFind(binary);
        if (cached != m_cache.constEnd()) {
            return cached.value();
        }
    }
    // Run --help outside the lock: it can take seconds and other binaries may be asked meanwhile.
    const QSet<QString> found = scan(m_environment.helpText(binary, environment));
    const QMutexLocker locker(&m_mutex);
    m_cache.insert(binary, found);
    return found;
}

QSet<QString> FlagProbe::scan(const QString &helpText) {
    QSet<QString> found;
    for (const QString &flag : knownFlags()) {
        if (helpText.contains(flag)) {
            found.insert(flag);
        }
    }
    return found;
}

QStringList FlagProbe::knownFlags() {
    return {QStringLiteral("--fit"),          QStringLiteral("--load-mode"),   QStringLiteral("--override-tensor"),
            QStringLiteral("--lazy-mode"),    QStringLiteral("--backend-sampling"), QStringLiteral("draft-mtp"),
            QStringLiteral("ngram-mod"),      QStringLiteral("--moe-hot"),     QStringLiteral("--alias"),
            QStringLiteral("--mmproj-device")};
}

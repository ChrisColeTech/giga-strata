#include "server_build.h"

#include "launch/server_env.h"
#include "launch/server_environment.h"

#include <QProcessEnvironment>

#include <utility>

ServerBuild::ServerBuild(const ServerEnvironment &environment, QString bundledBinary)
    : m_environment(environment), m_bundledBinary(std::move(bundledBinary)), m_probe(environment) {}

QString ServerBuild::binaryFor(const QString &overridePath) const {
    const QString trimmed = overridePath.trimmed();
    return trimmed.isEmpty() ? m_bundledBinary : trimmed;
}

bool ServerBuild::exists(const QString &binary) const {
    return m_environment.exists(binary) && !m_environment.isDirectory(binary);
}

QSet<QString> ServerBuild::flagsFor(const QString &binary) {
    // The scraped flags do not depend on the GPU ids (golden README section 10), so no GPUs.
    const QProcessEnvironment environment = ServerEnv::build(
        QProcessEnvironment::systemEnvironment(), ServerEnv::vendorDllDirectories(m_environment, binary), {});
    return m_probe.flags(binary, environment);
}

QSet<QString> ServerBuild::planningFlagsFor(const QString &binary) {
    if (!exists(binary)) {
        const QStringList known = FlagProbe::knownFlags();
        return QSet<QString>(known.cbegin(), known.cend());
    }
    return flagsFor(binary);
}

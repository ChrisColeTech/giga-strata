#pragma once

#include "launch/flag_probe.h"

#include <QSet>
#include <QString>

class ServerEnvironment;

// Which llama-server the app runs (04 section 6, 05 section 7): the bundled
// <app dir>\server\llama-server.exe, or the Settings override. Its optional flags are scraped
// from --help once per binary (detect_flags, D9) and cached; flagsFor() blocks, so callers run
// it on a worker thread. The environment is injected so tests serve canned help texts.
class ServerBuild {
public:
    ServerBuild(const ServerEnvironment &environment, QString bundledBinary);

    // `overridePath` is SettingsStore.serverOverride; empty means the bundled build.
    QString binaryFor(const QString &overridePath) const;
    bool exists(const QString &binary) const;

    // detect_flags. Thread-safe.
    QSet<QString> flagsFor(const QString &binary);

    // Live preview before a build exists (02 D3): plan as if every optional flag were present.
    QSet<QString> planningFlagsFor(const QString &binary);

    const QString &bundledBinary() const { return m_bundledBinary; }
    const ServerEnvironment &environment() const { return m_environment; }

private:
    const ServerEnvironment &m_environment;
    QString m_bundledBinary;
    FlagProbe m_probe;
};

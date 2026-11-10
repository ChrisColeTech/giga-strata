#pragma once

#include "launch/server_environment.h"

// ServerEnvironment over the real machine: filesystem, PATH and `--help` through QProcess.
class LocalServerEnvironment : public ServerEnvironment {
public:
    QString homeDirectory() const override;
    bool exists(const QString &path) const override;
    bool isDirectory(const QString &path) const override;
    QStringList listDirectory(const QString &directory) const override;
    // shutil.which as CPython 3.12 does it on Windows: current directory first unless
    // NoDefaultCurrentDirectoryInExePath says otherwise, PATHEXT extensions before the bare name.
    std::optional<QString> findOnPath(const QString &executableName) const override;
    QString helpText(const QString &binary, const QProcessEnvironment &environment) const override;
};

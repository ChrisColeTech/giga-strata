#pragma once

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

#include <optional>

// The machine as seen by server discovery and flag probing: candidate folders, PATH lookup and
// a build's --help text. Golden tests substitute a fake tree and captured help texts.
class ServerEnvironment {
public:
    virtual ~ServerEnvironment() = default;

    // os.path.expanduser("~")
    virtual QString homeDirectory() const = 0;
    // os.path.exists
    virtual bool exists(const QString &path) const = 0;
    // os.path.isdir
    virtual bool isDirectory(const QString &path) const = 0;
    // Entry names (not paths) directly inside `directory` in os.listdir order, which on NTFS is
    // the name order glob also returns; empty when the directory does not exist.
    virtual QStringList listDirectory(const QString &directory) const = 0;
    // shutil.which: full path of an executable found on PATH, if any.
    virtual std::optional<QString> findOnPath(const QString &executableName) const = 0;
    // stdout followed by stderr of `binary --help` run with `environment`; empty when it could
    // not run or timed out (detect_flags treats both as "no flags").
    virtual QString helpText(const QString &binary, const QProcessEnvironment &environment) const = 0;
};

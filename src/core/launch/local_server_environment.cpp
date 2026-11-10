#include "launch/local_server_environment.h"

#include "common/constants.h"
#include "common/model_names.h"
#include "launch/hidden_process.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QSet>

#include <algorithm>

#include <windows.h>

namespace {

const QString DEFAULT_PATHEXT = QStringLiteral(".COM;.EXE;.BAT;.CMD;.VBS;.JS;.WS;.MSC");

// ntpath.join for one component.
QString pythonJoin(const QString &directory, const QString &name) {
    if (directory.isEmpty() || directory.endsWith(u'\\') || directory.endsWith(u'/')
        || (directory.size() == 2 && directory.at(1) == u':')) {
        return directory + name;
    }
    return directory + u'\\' + name;
}

bool needsCurrentDirectory(const QString &executableName) {
    const std::wstring name = executableName.toStdWString();
    return NeedCurrentDirectoryForExePathW(name.c_str()) != FALSE;
}

QStringList candidateNames(const QString &executableName) {
    const QString pathExtension = qEnvironmentVariable("PATHEXT");
    const QString source = pathExtension.isEmpty() ? DEFAULT_PATHEXT : pathExtension;
    const QStringList extensions = source.split(u';', Qt::SkipEmptyParts);
    const qsizetype dot = executableName.lastIndexOf(u'.');
    const QString suffix = dot > 0 ? executableName.mid(dot).toUpper() : QString();
    QStringList names;
    for (const QString &extension : extensions) {
        names.append(executableName + extension);
    }
    // The bare name goes first only when it already carries an executable extension.
    const bool hasExecutableSuffix = std::any_of(extensions.cbegin(), extensions.cend(), [&](const QString &extension) {
        return suffix == extension.toUpper();
    });
    if (hasExecutableSuffix) {
        names.prepend(executableName);
    } else {
        names.append(executableName);
    }
    return names;
}

} // namespace

QString LocalServerEnvironment::homeDirectory() const {
    const QString profile = qEnvironmentVariable("USERPROFILE");
    if (!profile.isEmpty()) {
        return profile;
    }
    return qEnvironmentVariable("HOMEDRIVE") + qEnvironmentVariable("HOMEPATH");
}

bool LocalServerEnvironment::exists(const QString &path) const {
    return QFileInfo::exists(path);
}

bool LocalServerEnvironment::isDirectory(const QString &path) const {
    return QFileInfo(path).isDir();
}

QStringList LocalServerEnvironment::listDirectory(const QString &directory) const {
    // QDirIterator walks FindFirstFile order, which is os.listdir's order.
    QDirIterator it(directory, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    QStringList names;
    while (it.hasNext()) {
        it.next();
        names.append(it.fileName());
    }
    return names;
}

std::optional<QString> LocalServerEnvironment::findOnPath(const QString &executableName) const {
    if (executableName.contains(u'\\') || executableName.contains(u'/')) {
        return std::nullopt;
    }
    const QByteArray rawPath = qgetenv("PATH");
    if (qEnvironmentVariableIsSet("PATH") && rawPath.isEmpty()) {
        return std::nullopt;
    }
    // os.defpath when PATH is unset.
    QStringList directories{QStringLiteral("."), QStringLiteral("C:\\bin")};
    if (qEnvironmentVariableIsSet("PATH")) {
        directories = qEnvironmentVariable("PATH").split(u';');
    }
    if (needsCurrentDirectory(executableName)) {
        directories.prepend(QStringLiteral("."));
    }
    const QStringList names = candidateNames(executableName);
    QSet<QString> seen;
    for (const QString &directory : directories) {
        const QString key = ModelNames::normcase(directory);
        if (seen.contains(key)) {
            continue;
        }
        seen.insert(key);
        for (const QString &name : names) {
            const QString candidate = pythonJoin(directory, name);
            const QFileInfo info(candidate);
            if (info.exists() && !info.isDir()) {
                return candidate;
            }
        }
    }
    return std::nullopt;
}

QString LocalServerEnvironment::helpText(const QString &binary, const QProcessEnvironment &environment) const {
    QProcess process;
    HiddenProcess::configure(process);
    process.setProcessEnvironment(environment);
    process.start(binary, {QStringLiteral("--help")});
    if (!process.waitForStarted()) {
        return QString();
    }
    if (!process.waitForFinished(constants::FLAG_PROBE_TIMEOUT_SECONDS * 1000)) {
        process.kill();
        process.waitForFinished();
        return QString();
    }
    return QString::fromUtf8(process.readAllStandardOutput()) + QString::fromUtf8(process.readAllStandardError());
}

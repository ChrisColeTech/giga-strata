#include "about_info.h"

#include <QFile>
#include <QHash>
#include <QJSEngine>
#include <QTextStream>

namespace {

// BUILD_INFO.txt is the `key: value` lines scripts\build.bat writes (commit, branch, built).
QHash<QString, QString> readBuildInfo(const QString &path) {
    QHash<QString, QString> values;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return values;
    }
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        const qsizetype colon = line.indexOf(QLatin1Char(':'));
        if (colon > 0) {
            values.insert(line.left(colon).trimmed(), line.mid(colon + 1).trimmed());
        }
    }
    return values;
}

} // namespace

AboutInfo::AboutInfo(const QString &buildInfoPath, QObject *parent) : QObject(parent) {
    const QHash<QString, QString> values = readBuildInfo(buildInfoPath);
    m_serverCommit = values.value(QStringLiteral("commit"));
    m_serverBranch = values.value(QStringLiteral("branch"));
}

void AboutInfo::provide(AboutInfo *instance) {
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

AboutInfo *AboutInfo::create(QQmlEngine *, QJSEngine *) {
    Q_ASSERT_X(s_instance, "AboutInfo::create", "main.cpp must call AboutInfo::provide() before loading QML");
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

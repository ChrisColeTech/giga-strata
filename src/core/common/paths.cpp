#include "common/paths.h"

#include <QDir>
#include <QFileInfo>

namespace {

QString joined(const QString &root, const QString &relative) {
    return QDir::toNativeSeparators(QDir(root).filePath(relative));
}

} // namespace

Paths::Paths(const QString &projectRoot) : m_projectRoot(QDir::toNativeSeparators(QDir::cleanPath(projectRoot))) {}

QString Paths::findProjectRoot(const QString &startDirectory) {
    QDir directory(startDirectory);
    do {
        if (QFileInfo::exists(directory.filePath(QStringLiteral("src/main.cpp")))) {
            return directory.absolutePath();
        }
    } while (directory.cdUp());
    return QDir(startDirectory).absolutePath();
}

QString Paths::settingsFile() const {
    return joined(m_projectRoot, QStringLiteral("launcher_settings.json"));
}

QString Paths::usageDirectory() const {
    return joined(m_projectRoot, QStringLiteral("moe_usage"));
}

QString Paths::hotDirectory() const {
    return joined(m_projectRoot, QStringLiteral("moe_usage/hot"));
}

QString Paths::modelsDirectory() const {
    return joined(m_projectRoot, QStringLiteral("models"));
}

QString Paths::thirdPartyDirectory() const {
    return joined(m_projectRoot, QStringLiteral("third_party"));
}

#pragma once

#include "launch/server_environment.h"

#include <QMap>
#include <QSet>

// ServerEnvironment over a list of file paths (NTFS-style: case-insensitive), with folders
// implied by the files under them, a PATH list and canned --help texts.
class FakeServerEnvironment : public ServerEnvironment {
public:
    QString home;
    QStringList pathDirectories;
    QMap<QString, QString> helpTexts;  // binary -> text
    mutable int helpRuns = 0;

    // Entries are listed in the order files were added, like NTFS name order in the fixtures.
    void addFile(const QString &path) { m_files.append(path); }
    void addDirectory(const QString &path) { m_directories.append(path); }

    QString homeDirectory() const override { return home; }
    bool exists(const QString &path) const override { return isFile(path) || isDirectory(path); }
    bool isDirectory(const QString &path) const override {
        const QString prefix = path.toLower() + u'\\';
        for (const QString &file : m_files + m_directories) {
            if ((file + u'\\').toLower().startsWith(prefix)) {
                return true;
            }
        }
        return false;
    }
    QStringList listDirectory(const QString &directory) const override {
        const QString prefix = directory.toLower() + u'\\';
        QStringList names;
        QSet<QString> seen;
        for (const QString &file : m_files + m_directories) {
            if (!file.toLower().startsWith(prefix)) {
                continue;
            }
            const QString name = file.mid(prefix.size()).section(u'\\', 0, 0);
            if (!seen.contains(name.toLower())) {
                seen.insert(name.toLower());
                names.append(name);
            }
        }
        return names;
    }
    std::optional<QString> findOnPath(const QString &executableName) const override {
        for (const QString &directory : pathDirectories) {
            const QString candidate = directory + u'\\' + executableName + QStringLiteral(".EXE");
            if (isFile(candidate)) {
                return candidate;
            }
        }
        return std::nullopt;
    }
    QString helpText(const QString &binary, const QProcessEnvironment &) const override {
        ++helpRuns;
        return helpTexts.value(binary);
    }

private:
    bool isFile(const QString &path) const {
        for (const QString &file : m_files) {
            if (file.compare(path, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    }

    QStringList m_files;
    QStringList m_directories;
};

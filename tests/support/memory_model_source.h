#pragma once

#include "gguf/model_source.h"

#include <QMap>

#include <algorithm>

// ModelSource over in-memory files. A file may hold only its header bytes while reporting the
// real file's size, which is how golden fixtures stand in for 100 GB models.
class MemoryModelSource : public ModelSource {
public:
    struct File {
        QByteArray bytes;
        qint64 size = 0;
        qint64 modifiedTimeNs = 0;
    };

    void addFile(const QString &path, const QByteArray &bytes, qint64 size = -1, qint64 modifiedTimeNs = 0) {
        m_files.insert(path, {bytes, size < 0 ? bytes.size() : size, modifiedTimeNs});
    }
    void addDirectory(const QString &path) { m_directories.insert(path, true); }
    // Exact listFiles answer for one (directory, pattern, recursive) query, as recorded from Python.
    void addListing(const QString &directory, const QString &pattern, bool isRecursive, const QStringList &files) {
        m_listings.insert(listingKey(directory, pattern, isRecursive), files);
    }
    int prefixReadCount() const { return m_prefixReadCount; }

    bool exists(const QString &path) const override { return m_files.contains(path) || m_directories.contains(path); }
    bool isDirectory(const QString &path) const override { return m_directories.contains(path); }
    std::optional<qint64> fileSize(const QString &path) const override {
        return m_files.contains(path) ? std::optional<qint64>(m_files[path].size) : std::nullopt;
    }
    std::optional<qint64> modifiedTimeNs(const QString &path) const override {
        return m_files.contains(path) ? std::optional<qint64>(m_files[path].modifiedTimeNs) : std::nullopt;
    }
    std::optional<QByteArray> readPrefix(const QString &path, qint64 maxBytes) const override {
        if (!m_files.contains(path)) {
            return std::nullopt;
        }
        ++m_prefixReadCount;
        return m_files[path].bytes.left(qsizetype(std::min<qint64>(maxBytes, m_files[path].bytes.size())));
    }
    QStringList listFiles(const QString &directory, const QString &pattern, bool isRecursive) const override {
        return m_listings.value(listingKey(directory, pattern, isRecursive));
    }

private:
    static QString listingKey(const QString &directory, const QString &pattern, bool isRecursive) {
        return directory + u'|' + pattern + u'|' + (isRecursive ? u'r' : u'f');
    }

    QMap<QString, File> m_files;
    QMap<QString, bool> m_directories;
    QMap<QString, QStringList> m_listings;
    mutable int m_prefixReadCount = 0;
};

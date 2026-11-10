#include "gguf/local_model_source.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include <windows.h>

#include <algorithm>

bool LocalModelSource::exists(const QString &path) const {
    return QFileInfo::exists(path);
}

bool LocalModelSource::isDirectory(const QString &path) const {
    return QFileInfo(path).isDir();
}

std::optional<qint64> LocalModelSource::fileSize(const QString &path) const {
    const QFileInfo info(path);
    if (!info.isFile()) {
        return std::nullopt;
    }
    return info.size();
}

std::optional<qint64> LocalModelSource::modifiedTimeNs(const QString &path) const {
    // Python's st_mtime_ns is the raw NTFS 100 ns timestamp; Qt only offers milliseconds,
    // which would change every hot-cache key (02 H9, H10).
    constexpr qint64 TICKS_FROM_1601_TO_1970 = 116444736000000000LL;
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    const auto nativePath = QDir::toNativeSeparators(path).toStdWString();
    if (!GetFileAttributesExW(nativePath.c_str(), GetFileExInfoStandard, &attributes)) {
        return std::nullopt;
    }
    const qint64 ticks = (qint64(attributes.ftLastWriteTime.dwHighDateTime) << 32) | attributes.ftLastWriteTime.dwLowDateTime;
    return (ticks - TICKS_FROM_1601_TO_1970) * 100;
}

std::optional<QByteArray> LocalModelSource::readPrefix(const QString &path, qint64 maxBytes) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    return file.read(maxBytes);
}

QStringList LocalModelSource::listFiles(const QString &directory, const QString &pattern, bool isRecursive) const {
    // Python glob: case-insensitive on Windows, skips dot-names, joins with the directory as
    // given, then the caller sorts by plain string order.
    const auto flags = isRecursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    QDirIterator it(directory, {pattern}, QDir::Files, flags);
    const QString root = QDir::toNativeSeparators(directory);
    const QDir base(directory);
    QStringList files;
    while (it.hasNext()) {
        const QString found = it.next();
        if (it.fileName().startsWith(u'.')) {
            continue;
        }
        files.append(root + QDir::separator() + QDir::toNativeSeparators(base.relativeFilePath(found)));
    }
    std::sort(files.begin(), files.end());
    return files;
}

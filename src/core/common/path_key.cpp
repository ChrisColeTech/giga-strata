#include "common/path_key.h"

#include <QDir>
#include <QFileInfo>

namespace {

QString absoluteNative(const QString &path) {
    return QDir::toNativeSeparators(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
}

} // namespace

QString path_key::of(const QString &path) {
    return absoluteNative(path).toLower();
}

QString path_key::parentDirectory(const QString &path) {
    const QString absolute = absoluteNative(path);
    const qsizetype cut = absolute.lastIndexOf(u'\\');
    // "N:\file" keeps its root separator, like Python's dirname.
    return cut <= 2 ? absolute.left(cut + 1) : absolute.left(cut);
}

QString path_key::join(const QString &directory, const QString &name) {
    if (directory.isEmpty() || directory.endsWith(u'\\') || directory.endsWith(u'/')) {
        return directory + name;
    }
    return directory + u'\\' + name;
}

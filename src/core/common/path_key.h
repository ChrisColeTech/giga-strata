#pragma once

#include <QString>

// Python os.path.normcase(os.path.abspath(p)) on Windows: absolute, collapsed, backslashes,
// lowercase. The reference uses it as file identity in caches and de-duplication.
namespace path_key {

QString of(const QString &path);

// os.path.dirname(os.path.abspath(p)) with native separators, case preserved.
QString parentDirectory(const QString &path);

// os.path.join for one more component, native separators.
QString join(const QString &directory, const QString &name);

} // namespace path_key

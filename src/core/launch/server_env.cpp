#include "launch/server_env.h"

#include "common/model_names.h"
#include "common/path_key.h"
#include "launch/server_environment.h"

#include <QDir>

namespace {

// os.path.dirname(os.path.abspath(path)). Unlike QFileInfo, Python keeps the drive letter's case
// ("c:\users"), and the folders found here end up verbatim in the server's PATH.
QString pythonParentDirectory(const QString &path) {
    const QString absolute = QDir::isAbsolutePath(path) ? path : QDir::current().absoluteFilePath(path);
    const QString clean = QDir::toNativeSeparators(QDir::cleanPath(absolute));
    const qsizetype cut = clean.lastIndexOf(u'\\');
    return cut <= 2 ? clean.left(cut + 1) : clean.left(cut);
}

} // namespace

QStringList ServerEnv::vendorDllDirectories(const ServerEnvironment &environment, const QString &binary) {
    const QString exeDirectory = pythonParentDirectory(binary);
    const QString vendor = path_key::join(pythonParentDirectory(exeDirectory), QStringLiteral("vendor"));
    const QString name = ModelNames::baseName(exeDirectory).toLower();
    const QString cuda12 = QStringLiteral("cuda12");
    const QString kind = name.contains(cuda12) ? cuda12 : name.contains(QStringLiteral("cuda")) ? QStringLiteral("cuda")
                                                                                                  : QString();
    if (kind.isEmpty() || !environment.isDirectory(vendor)) {
        return {};
    }
    QStringList directories;
    for (const QString &entry : environment.listDirectory(vendor)) {
        const QString folded = entry.toLower();
        if (folded.contains(kind) && (kind == cuda12 || !folded.contains(cuda12))) {
            directories.append(path_key::join(vendor, entry));
        }
    }
    return directories;
}

QProcessEnvironment ServerEnv::build(const QProcessEnvironment &base, const QStringList &vendorDirectories,
                                     const QVector<int> &gpuIds) {
    QProcessEnvironment environment = base;
    // Make CUDA numbering match nvidia-smi, which is how the user picks GPUs.
    environment.insert(QStringLiteral("CUDA_DEVICE_ORDER"), QStringLiteral("PCI_BUS_ID"));
    QStringList ids;
    for (const int id : gpuIds) {
        ids.append(QString::number(id));
    }
    environment.insert(QStringLiteral("CUDA_VISIBLE_DEVICES"), ids.join(u','));
    if (!vendorDirectories.isEmpty()) {
        environment.insert(QStringLiteral("PATH"),
                           (vendorDirectories + QStringList{base.value(QStringLiteral("PATH"))}).join(u';'));
    }
    return environment;
}

QProcessEnvironment ServerEnv::forProbe(const QProcessEnvironment &base, const QStringList &vendorDirectories) {
    QProcessEnvironment environment = build(base, vendorDirectories, {});
    environment.insert(QStringLiteral("CUDA_VISIBLE_DEVICES"), QStringLiteral("-1"));
    return environment;
}

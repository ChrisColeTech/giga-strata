#include "launch/server_finder.h"

#include "common/model_names.h"
#include "common/path_key.h"
#include "common/python_int.h"
#include "launch/server_environment.h"

#include <QPair>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <vector>

namespace {

const QString SERVER_EXE = QStringLiteral("llama-server.exe");

// os.path.basename(os.path.dirname(path)): the folder a build's exe sits in.
QString buildFolderName(const QString &path) {
    const qsizetype cut = std::max(path.lastIndexOf(u'\\'), path.lastIndexOf(u'/'));
    QString parent = cut < 0 ? QString() : path.left(cut);
    while (parent.size() > 1 && (parent.endsWith(u'\\') || parent.endsWith(u'/'))) {
        parent.chop(1);
    }
    return ModelNames::baseName(parent);
}

QString joinAll(const QString &root, const QStringList &parts) {
    QString path = root;
    for (const QString &part : parts) {
        path = path_key::join(path, part);
    }
    return path;
}

// glob(<backends>\llama.cpp-*nvidia-cuda*\llama-server.exe): directories only, no dot-names,
// matched case-insensitively (fnmatch normcases on Windows), in listdir order.
QStringList lmStudioServers(const ServerEnvironment &environment) {
    const QString backends =
        joinAll(environment.homeDirectory(), {QStringLiteral(".lmstudio"), QStringLiteral("extensions"),
                                              QStringLiteral("backends")});
    const QString prefix = QStringLiteral("llama.cpp-");
    QStringList found;
    for (const QString &name : environment.listDirectory(backends)) {
        const QString folded = name.toLower();
        if (name.startsWith(u'.') || !folded.startsWith(prefix)
            || folded.indexOf(QStringLiteral("nvidia-cuda"), prefix.size()) < 0) {
            continue;
        }
        const QString folder = path_key::join(backends, name);
        const QString exe = path_key::join(folder, SERVER_EXE);
        if (environment.isDirectory(folder) && environment.exists(exe)) {
            found.append(exe);
        }
    }
    return found;
}

std::vector<qint64> versionOf(const QString &path) {
    const QVector<qint64> key = ServerFinder::versionKey(path);
    return std::vector<qint64>(key.cbegin(), key.cend());
}

// Newest build per family, in first-seen order like Python's dict.
QStringList newestPerFamily(const QStringList &servers) {
    static const QRegularExpression SUPPORTED_CUDA(QStringLiteral("cuda1[2-9]"));
    QVector<QPair<QString, QString>> newest;
    for (const QString &exe : servers) {
        const QString folder = buildFolderName(exe);
        const qsizetype dash = folder.lastIndexOf(u'-');
        const QString family = dash < 0 ? folder : folder.left(dash);
        // CUDA 11 builds cannot drive RTX 50-series cards and hang on startup instead of failing.
        if (!SUPPORTED_CUDA.match(family).hasMatch()) {
            continue;
        }
        const auto known = std::find_if(newest.begin(), newest.end(),
                                        [&](const QPair<QString, QString> &entry) { return entry.first == family; });
        if (known == newest.end()) {
            newest.append({family, exe});
        } else if (versionOf(exe) > versionOf(known->second)) {
            known->second = exe;
        }
    }
    QStringList ordered;
    for (const auto &entry : newest) {
        ordered.append(entry.second);
    }
    return ordered;
}

std::vector<qint64> negated(const QString &path) {
    std::vector<qint64> key = versionOf(path);
    std::transform(key.begin(), key.end(), key.begin(), [](qint64 value) { return -value; });
    return key;
}

} // namespace

QStringList ServerFinder::find(const ServerEnvironment &environment, const QString &upstreamServer,
                               const QString &builtServer) {
    QStringList found;
    if (environment.exists(upstreamServer)) {
        found.append(upstreamServer);
    }
    QStringList lmStudio = newestPerFamily(lmStudioServers(environment));
    std::stable_sort(lmStudio.begin(), lmStudio.end(), [](const QString &a, const QString &b) {
        const bool aIsOther = !a.contains(QStringLiteral("cuda12"));
        const bool bIsOther = !b.contains(QStringLiteral("cuda12"));
        if (aIsOther != bIsOther) {
            return !aIsOther;
        }
        return negated(a) < negated(b);
    });
    found += lmStudio;
    if (environment.exists(builtServer)) {
        found.append(builtServer);
    }
    if (const auto onPath = environment.findOnPath(QStringLiteral("llama-server"))) {
        found.append(*onPath);
    }
    QStringList unique;
    QSet<QString> seen;
    for (const QString &path : found) {
        const QString key = path_key::of(path);
        if (!seen.contains(key)) {
            seen.insert(key);
            unique.append(key);
        }
    }
    return unique;
}

QString ServerFinder::upstreamServer(const QString &projectRoot) {
    return joinAll(projectRoot, {QStringLiteral("third_party"), QStringLiteral("llama.cpp-upstream"),
                                 QStringLiteral("bin"), QStringLiteral("bin"), QStringLiteral("Release"), SERVER_EXE});
}

QString ServerFinder::builtServer(const QString &projectRoot) {
    return joinAll(projectRoot, {QStringLiteral("third_party"), QStringLiteral("llama.cpp-glm5"), QStringLiteral("bin"),
                                 QStringLiteral("bin"), QStringLiteral("Release"), SERVER_EXE});
}

QString ServerFinder::defaultBinary(const ServerEnvironment &environment, const QString &projectRoot) {
    for (const QString &candidate : {upstreamServer(projectRoot), builtServer(projectRoot)}) {
        if (environment.exists(candidate)) {
            return candidate;
        }
    }
    return QStringLiteral("llama-server");
}

QVector<qint64> ServerFinder::versionKey(const QString &serverPath) {
    const QString folder = buildFolderName(serverPath);
    const QString tail = folder.mid(folder.lastIndexOf(u'-') + 1);
    QVector<qint64> key;
    for (const QString &part : tail.split(u'.')) {
        const auto value = python_int::parse(part);
        if (!value) {
            return {};
        }
        key.append(*value);
    }
    return key;
}

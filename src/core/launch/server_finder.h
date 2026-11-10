#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

class ServerEnvironment;

// llama_launcher.find_servers: candidate llama-server builds, most generally useful first. The
// list must equal Python's exactly (order and normcase) or the remembered server picks in
// launcher_settings.json are dropped (02 H13).
class ServerFinder {
public:
    // This project's upstream build, the newest LM Studio CUDA 12+ build per family (CUDA 12
    // first, then newer versions first), the GLM-5.3 fork, then PATH; normcase(abspath), deduped.
    static QStringList find(const ServerEnvironment &environment, const QString &upstreamServer,
                            const QString &builtServer);

    // _UPSTREAM_SERVER / _BUILT_SERVER under a project root.
    static QString upstreamServer(const QString &projectRoot);
    static QString builtServer(const QString &projectRoot);

    // DEFAULT_BINARY: the first of upstream and built that exists, else "llama-server".
    static QString defaultBinary(const ServerEnvironment &environment, const QString &projectRoot);

    // _version_key: the dotted version after the last "-" of the build's folder name, parsed with
    // Python int(); empty (Python's ()) when any part does not parse.
    static QVector<qint64> versionKey(const QString &serverPath);
};

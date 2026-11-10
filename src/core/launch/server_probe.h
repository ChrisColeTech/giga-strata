#pragma once

#include "common/constants.h"

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

#include <atomic>
#include <optional>

// probe_server's answer: whether this build loaded the model, and why not.
struct ProbeVerdict {
    bool isLoadable = false;
    QString reason;  // "ok", the server's own error text, or why the probe gave up
};

// llama_launcher.probe_server: trial-load the model on the CPU only and stop as soon as the
// server is listening or gives up. Blocking; run it on a worker thread (R27). A build that does
// not understand the file fails within a second or two.
class ServerProbe {
public:
    // `environment` should come from ServerEnv::forProbe (no GPUs, 02 D7). The child is killed,
    // never asked to close (02 D6), on success, failure, timeout and cancel alike.
    static ProbeVerdict probe(const QString &binary, const QString &modelPath, const QProcessEnvironment &environment,
                              int timeoutSeconds = constants::PROBE_TIMEOUT_SECONDS,
                              const std::atomic<bool> *cancel = nullptr);

    // -m <model> -ngl 0 -c 256 -np 1 --no-warmup --host 127.0.0.1 --port <port>
    static QStringList arguments(const QString &modelPath, int port);

    // The verdict one output line settles: "listening" means loaded; "error loading model" or
    // "unknown model architecture" means not, with the text after "error loading model:".
    static std::optional<ProbeVerdict> verdictForLine(const QString &line);

    // The reason Python reports when the output ends (exit or timeout) with neither.
    static QString exitedWithoutLoading();
};

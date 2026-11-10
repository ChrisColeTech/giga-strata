#pragma once

#include "common/result.h"

#include <QByteArray>
#include <QProcessEnvironment>
#include <QSet>
#include <QString>
#include <QStringList>

#include <optional>

struct ModelLayout;

// What set_usage_env decided: whether the server records expert usage, to which file, whether
// an existing file of another model has to be moved to <path>.bak first, and the "[MOE] ..."
// lines run_process logs, in its order, each ending in "\n".
struct UsageRecording {
    bool isRecording = false;
    bool needsBackup = false;
    QString usagePath;
    QString flushTokens;
    QStringList logLines;
};

// llama_launcher.set_usage_env (FR-1.6). The decision is pure; prepare() is the small I/O step
// that reads the existing file, moves it aside and fills the environment.
class UsageEnv {
public:
    // The compatibility guard: a missing file (`exists` false) is fine; an unreadable one, invalid
    // JSON, or a file recorded for another (arch, n_expert, n_layer + n_skipped) is not.
    static bool isCompatible(bool exists, const std::optional<QByteArray> &contents, const ModelLayout &layout);

    // `flags` empty means the --help scrape failed; Python then skips the "predates" warning.
    static UsageRecording plan(const ModelLayout &layout, const QSet<QString> &flags, const QString &usagePath,
                               bool isCompatible, const QProcessEnvironment &environment);

    // LLAMA_MOE_USAGE_OUT, and LLAMA_MOE_USAGE_FLUSH unless the environment already sets it.
    static void apply(const UsageRecording &recording, QProcessEnvironment &environment);

    // Run-time: create the usage folder, read and judge the existing file, os.replace it to
    // <path>.bak when needed, then apply. Fails only when the move fails (Python aborts the run).
    static Result<UsageRecording> prepare(const ModelLayout &layout, const QSet<QString> &flags,
                                          const QString &usageDirectory, const QString &usagePath,
                                          QProcessEnvironment &environment);
};

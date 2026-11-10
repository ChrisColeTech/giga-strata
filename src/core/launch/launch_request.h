#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

#include <optional>

// Everything run_process needs besides the plan to assemble the llama-server command.
struct LaunchRequest {
    QString binary;
    QString modelPath;
    QString host;
    QString port;
    QStringList extraArgs;              // already tokenized like shlex.split(posix=False)
    bool useFlashAttention = true;
    int gpuCount = 0;                   // the draft and projector go to CUDA<gpuCount - 1>
    QString alias;                      // model stem, sent as --alias unless extra args set one
    std::optional<QString> draftPath;   // MTP draft, already checked against flags and arch
    std::optional<QString> mmprojPath;
    QSet<QString> flags;                // optional flags this build understands (detect_flags)
};

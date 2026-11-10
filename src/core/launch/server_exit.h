#pragma once

#include "launch/log_summary.h"

#include <QString>
#include <QStringList>

// How a server run ended, for the status pill (FR-4.3) and the toast: failed or not, and one
// user sentence.
struct ServerOutcome {
    bool isFailure = false;
    QString message;
};

// The end of a run comes from the process exit code, not the log: a killed server prints nothing,
// a missing DLL (0xC0000135) prints nothing at all, a crash (0xC0000005) stops mid-line. This maps
// the exit code plus what the log said into the final state.
class ServerExit {
public:
    // exitCode as Windows reports it (QProcess::exitCode() cast to quint32). wasStopRequested:
    // the app killed it (Stop, quit), which always reads as a normal stop.
    static ServerOutcome interpret(quint32 exitCode, bool wasStopRequested, const LogSummary &summary);

    // run_process's own lines after the exit, verbatim: "\n[LAUNCHER] Server exited with code N\n"
    // and, for 0xC0000135, the missing-DLL hint line.
    static QStringList launcherLines(quint32 exitCode);

    // The missing-DLL hint sentence run_process prints (without its "[LAUNCHER] " prefix).
    static QString missingDllHint();
};

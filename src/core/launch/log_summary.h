#pragma once

#include <QString>

// What the server's output said before it exited; ServerExit turns it plus the exit code into
// the final state (FR-4.3). LogParser keeps it up to date.
struct LogSummary {
    bool hasStartedLoading = false;
    bool isListening = false;
    QString listeningUrl;
    QString loadError;      // first reason of the latest load
    QString startupError;   // LM Studio json message
    QString argumentError;  // "invalid argument: --x", "error while handling argument ..."
    QString lastErrorLine;  // fallback reason: the last line classified as an error
};

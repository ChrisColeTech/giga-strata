#include "launch/server_exit.h"

#include "common/constants.h"

namespace {

constexpr quint32 EXIT_CODE_ACCESS_VIOLATION = 0xC0000005;
constexpr quint32 FIRST_NTSTATUS_ERROR = 0xC0000000;

QString codeText(quint32 exitCode) {
    // NTSTATUS crash codes read better in hex, the way Windows documents them.
    if (exitCode >= FIRST_NTSTATUS_ERROR) {
        return QStringLiteral("0x") + QString::number(exitCode, 16).toUpper().rightJustified(8, u'0');
    }
    return QString::number(exitCode);
}

} // namespace

ServerOutcome ServerExit::interpret(quint32 exitCode, bool wasStopRequested, const LogSummary &summary) {
    if (exitCode == constants::EXIT_CODE_DLL_NOT_FOUND) {
        return {true, missingDllHint()};
    }
    if (wasStopRequested) {
        return {false, QStringLiteral("Server stopped.")};
    }
    if (!summary.loadError.isEmpty() || !summary.startupError.isEmpty()) {
        const QString reason = summary.loadError.isEmpty() ? summary.startupError : summary.loadError;
        return {true, QStringLiteral("The server could not load the model: %1").arg(reason)};
    }
    if (!summary.argumentError.isEmpty()) {
        return {true, QStringLiteral("The server rejected its arguments: %1").arg(summary.argumentError)};
    }
    const bool isLoading = summary.hasStartedLoading && !summary.isListening;
    if (exitCode == EXIT_CODE_ACCESS_VIOLATION) {
        return {true, isLoading ? QStringLiteral("The server crashed while loading the model (access violation, "
                                                 "0xC0000005). This build probably cannot load this file.")
                                : QStringLiteral("The server crashed (access violation, 0xC0000005).")};
    }
    if (exitCode == 0) {
        return {false, QStringLiteral("Server exited.")};
    }
    if (summary.isListening) {
        return {true, QStringLiteral("The server stopped unexpectedly (exit code %1).").arg(codeText(exitCode))};
    }
    return {true, QStringLiteral("The server exited with code %1 before it was ready.").arg(codeText(exitCode))};
}

QStringList ServerExit::launcherLines(quint32 exitCode) {
    QStringList lines{QStringLiteral("\n[LAUNCHER] Server exited with code %1\n").arg(exitCode)};
    if (exitCode == constants::EXIT_CODE_DLL_NOT_FOUND) {
        lines.append(QStringLiteral("[LAUNCHER] ") + missingDllHint() + u'\n');
    }
    return lines;
}

QString ServerExit::missingDllHint() {
    return QStringLiteral("Windows could not find a DLL the server needs (e.g. the CUDA runtime). Put the folder with "
                          "cudart64_*.dll on PATH or next to llama-server.exe.");
}

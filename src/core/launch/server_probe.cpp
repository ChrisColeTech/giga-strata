#include "launch/server_probe.h"

#include "common/python_text.h"
#include "launch/free_port.h"
#include "launch/hidden_process.h"
#include "launch/line_splitter.h"

#include <QDeadlineTimer>
#include <QProcess>

namespace {

constexpr int POLL_MS = 100;

std::optional<ProbeVerdict> firstVerdict(const QStringList &lines) {
    for (const QString &line : lines) {
        if (const auto verdict = ServerProbe::verdictForLine(line)) {
            return verdict;
        }
    }
    return std::nullopt;
}

void stop(QProcess &process) {
    process.kill();
    process.waitForFinished();
}

} // namespace

ProbeVerdict ServerProbe::probe(const QString &binary, const QString &modelPath,
                                const QProcessEnvironment &environment, int timeoutSeconds,
                                const std::atomic<bool> *cancel) {
    const auto port = FreePort::pick();
    if (!port.ok()) {
        return {false, port.error()};
    }
    QProcess process;
    HiddenProcess::configure(process);
    process.setProcessEnvironment(environment);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(binary, arguments(modelPath, port.value()));
    if (!process.waitForStarted()) {
        return {false, QStringLiteral("cannot start it: %1").arg(process.errorString())};
    }
    LineSplitter splitter;
    const QDeadlineTimer deadline{std::chrono::seconds(timeoutSeconds)};
    while (!deadline.hasExpired() && !(cancel && cancel->load())) {
        const bool isFinished = !process.waitForReadyRead(POLL_MS) && process.state() == QProcess::NotRunning;
        QStringList lines = splitter.feed(process.readAll());
        if (isFinished) {
            lines += splitter.finish();
        }
        if (const auto verdict = firstVerdict(lines)) {
            stop(process);
            return *verdict;
        }
        if (isFinished) {
            return {false, exitedWithoutLoading()};
        }
    }
    stop(process);
    if (cancel && cancel->load()) {
        return {false, QStringLiteral("the trial load was cancelled")};
    }
    return {false, exitedWithoutLoading()};
}

QStringList ServerProbe::arguments(const QString &modelPath, int port) {
    return {QStringLiteral("-m"),      modelPath,
            QStringLiteral("-ngl"),    QStringLiteral("0"),
            QStringLiteral("-c"),      QStringLiteral("256"),
            QStringLiteral("-np"),     QStringLiteral("1"),
            QStringLiteral("--no-warmup"),
            QStringLiteral("--host"),  QStringLiteral("127.0.0.1"),
            QStringLiteral("--port"),  QString::number(port)};
}

std::optional<ProbeVerdict> ServerProbe::verdictForLine(const QString &line) {
    if (line.contains(QStringLiteral("listening"))) {
        return ProbeVerdict{true, QStringLiteral("ok")};
    }
    if (line.contains(QStringLiteral("error loading model"))
        || line.contains(QStringLiteral("unknown model architecture"))) {
        const QString marker = QStringLiteral("error loading model:");
        const QString stripped = python_text::strip(line);
        const qsizetype at = stripped.lastIndexOf(marker);
        return ProbeVerdict{false, python_text::strip(at < 0 ? stripped : stripped.mid(at + marker.size()))};
    }
    return std::nullopt;
}

QString ServerProbe::exitedWithoutLoading() {
    return QStringLiteral("server exited without loading the model");
}

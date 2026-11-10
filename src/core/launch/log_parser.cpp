#include "launch/log_parser.h"

#include "launch/log_line_classifier.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <algorithm>

namespace {

// "0.03.543.934 I " -> minutes, seconds, milliseconds, microseconds, level.
const QRegularExpression TIMESTAMP(QStringLiteral("^(\\d+)\\.(\\d{2})\\.(\\d{3})\\.(\\d{3}) [A-Z] "));
const QRegularExpression LOADING(QStringLiteral("load_model: loading model '(.*)'"));
const QRegularExpression LISTENING(QStringLiteral("listening on (https?://\\S+)"));
const QRegularExpression TIMING_HEADER(QStringLiteral("print_timing: id\\s+\\d+ \\| task (-?\\d+) \\|\\s*"));
// Anchored after the optional prefixes so "prompt eval time" never reads as "eval time".
const QRegularExpression TIMING(QStringLiteral("^\\s*(prompt eval|eval) time =\\s*([\\d.]+) ms /\\s*(\\d+) tokens "
                                               "\\(\\s*[\\d.]+ ms per token,\\s*([\\d.]+) tokens per second\\)"));
const QString LOAD_ERROR = QStringLiteral("error loading model:");
const QString GGUF_INIT = QStringLiteral("gguf_init_from_file_ptr: ");
const QString STARTUP_ERROR = QStringLiteral("LMSTUDIO_STARTUP_ERROR:");

LogEvent typed(LogEvent::Type type, const QString &text = QString()) {
    LogEvent event;
    event.type = type;
    event.text = text;
    return event;
}

bool isDotRun(const QByteArray &bytes) {
    return !bytes.isEmpty() && std::all_of(bytes.cbegin(), bytes.cend(), [](char byte) { return byte == '.'; });
}

} // namespace

QVector<LogEvent> LogParser::feed(const QByteArray &chunk) {
    QVector<LogEvent> events;
    for (const QString &line : m_splitter.feed(chunk)) {
        parseLine(line, events);
    }
    reportDots(events);
    return events;
}

QVector<LogEvent> LogParser::finish() {
    QVector<LogEvent> events;
    for (const QString &line : m_splitter.finish()) {
        parseLine(line, events);
    }
    return events;
}

void LogParser::reportDots(QVector<LogEvent> &events) {
    // glm5 writes one dot per ~1% of tensor data without a newline; count them as they come.
    const QByteArray &pending = m_splitter.pending();
    if (!isDotRun(pending) || int(pending.size()) == m_reportedDots) {
        return;
    }
    m_reportedDots = int(pending.size());
    LogEvent progress = typed(LogEvent::Type::LoadProgress);
    progress.progressPercent = std::min(m_reportedDots, 99);
    events.append(progress);
}

void LogParser::parseLine(const QString &line, QVector<LogEvent> &events) {
    LogEvent lineEvent = typed(LogEvent::Type::Line, line);
    lineEvent.kind = LogLineClassifier::classify(line);
    const QRegularExpressionMatch stamp = TIMESTAMP.match(line);
    if (stamp.hasMatch()) {
        lineEvent.elapsedMs = stamp.captured(1).toLongLong() * 60000 + stamp.captured(2).toLongLong() * 1000
                              + stamp.captured(3).toLongLong();
    }
    events.append(lineEvent);
    if (lineEvent.kind == LogEvent::Kind::Error) {
        m_summary.lastErrorLine = line;
    }
    const QString body = stamp.hasMatch() ? line.mid(stamp.capturedLength()) : line;
    if (isDotRun(body.toLatin1()) && !stamp.hasMatch()) {
        LogEvent progress = typed(LogEvent::Type::LoadProgress);
        progress.progressPercent = 100;
        events.append(progress);
        m_reportedDots = 0;
        return;
    }
    m_reportedDots = 0;
    parseLoading(body, events);
    parseTiming(body, events);
    parseFailure(body, stamp.hasMatch(), events);
}

void LogParser::parseLoading(const QString &body, QVector<LogEvent> &events) {
    const QRegularExpressionMatch loading = LOADING.match(body);
    if (loading.hasMatch()) {
        m_summary.hasStartedLoading = true;
        m_summary.loadError.clear();
        m_ggufReason.clear();
        m_isLoadErrorReported = false;
        events.append(typed(LogEvent::Type::LoadingStarted, loading.captured(1)));
        return;
    }
    const QRegularExpressionMatch listening = LISTENING.match(body);
    if (listening.hasMatch()) {
        m_summary.isListening = true;
        m_summary.listeningUrl = listening.captured(1);
        events.append(typed(LogEvent::Type::Listening, m_summary.listeningUrl));
    }
}

void LogParser::parseTiming(const QString &body, QVector<LogEvent> &events) {
    QString rest = body;
    const QRegularExpressionMatch header = TIMING_HEADER.match(body);
    if (header.hasMatch()) {
        m_timingTaskId = header.captured(1).toInt();
        rest = body.mid(header.capturedEnd());
    }
    const QRegularExpressionMatch timing = TIMING.match(rest);
    if (!timing.hasMatch()) {
        return;
    }
    LogEvent event = typed(LogEvent::Type::RequestTiming);
    event.isPrompt = timing.captured(1) == QStringLiteral("prompt eval");
    event.taskId = m_timingTaskId;
    event.milliseconds = timing.captured(2).toDouble();
    event.tokens = timing.captured(3).toLongLong();
    event.tokensPerSecondText = timing.captured(4);
    event.tokensPerSecond = event.tokensPerSecondText.toDouble();
    events.append(event);
}

void LogParser::parseFailure(const QString &body, bool isPrefixed, QVector<LogEvent> &events) {
    if (body.startsWith(STARTUP_ERROR)) {
        const QJsonObject json = QJsonDocument::fromJson(body.mid(STARTUP_ERROR.size()).toUtf8()).object();
        LogEvent event = typed(LogEvent::Type::StartupError, json.value(QStringLiteral("message")).toString());
        event.code = json.value(QStringLiteral("code")).toString();
        m_summary.startupError = event.text;
        events.append(event);
        return;
    }
    const qsizetype gguf = body.indexOf(GGUF_INIT);
    if (gguf >= 0 && m_ggufReason.isEmpty()) {
        m_ggufReason = body.mid(gguf + GGUF_INIT.size()).trimmed();
    }
    const qsizetype loadError = body.indexOf(LOAD_ERROR);
    const bool isFailedLoad = body.contains(QStringLiteral("load_model: failed to load model"));
    if ((loadError >= 0 || isFailedLoad) && !m_isLoadErrorReported) {
        // Every load error is printed twice (fit pre-pass, then the real load): report the first.
        // glm5 puts the useful reason for a bad tensor on the gguf_init line, not this one.
        const QString own = loadError >= 0 ? body.mid(loadError + LOAD_ERROR.size()).trimmed()
                                           : QStringLiteral("the server could not load the model");
        m_summary.loadError = m_ggufReason.isEmpty() ? own : m_ggufReason;
        m_isLoadErrorReported = true;
        events.append(typed(LogEvent::Type::LoadError, m_summary.loadError));
        return;
    }
    const bool isArgumentError =
        body.startsWith(QStringLiteral("error: ")) || body.startsWith(QStringLiteral("error while handling argument"));
    if (!isPrefixed && isArgumentError && !m_summary.hasStartedLoading && m_summary.argumentError.isEmpty()) {
        m_summary.argumentError = body.startsWith(QStringLiteral("error: ")) ? body.mid(7).trimmed() : body.trimmed();
        events.append(typed(LogEvent::Type::ArgumentError, m_summary.argumentError));
    }
}

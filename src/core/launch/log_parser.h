#pragma once

#include "launch/line_splitter.h"
#include "launch/log_event.h"
#include "launch/log_summary.h"

#include <QByteArray>
#include <QVector>

// Incremental reader of merged llama-server stdout+stderr (P7, FR-4.3/4.4). Feed it raw chunks as
// they arrive; it splits lines like Python's text pipe ("\r\n", "\n", "\r"), reports glm5's
// progress dots while their line is still open, and recognizes the upstream / LM Studio logger
// ("M.SS.mmm.uuu L tag func: msg") as well as glm5's prefix-less one. Line patterns are those in
// tests/golden/fixtures/logs/README.md.
class LogParser {
public:
    QVector<LogEvent> feed(const QByteArray &chunk);

    // At process exit: the last unterminated line, if any.
    QVector<LogEvent> finish();

    const LogSummary &summary() const { return m_summary; }

private:
    void parseLine(const QString &line, QVector<LogEvent> &events);
    void parseLoading(const QString &body, QVector<LogEvent> &events);
    void parseTiming(const QString &body, QVector<LogEvent> &events);
    void parseFailure(const QString &body, bool isPrefixed, QVector<LogEvent> &events);
    void reportDots(QVector<LogEvent> &events);

    LineSplitter m_splitter;
    LogSummary m_summary;
    QString m_ggufReason;      // first gguf_init_from_file_ptr message of the current load
    bool m_isLoadErrorReported = false;
    int m_timingTaskId = -1;   // glm5 prints the task id on a header line above its timing lines
    int m_reportedDots = 0;
};

#include "launch/log_line_classifier.h"

#include <QRegularExpression>
#include <QStringList>

#include <optional>

namespace {

const QStringList LAUNCHER_PREFIXES{QStringLiteral("[LAUNCHER]"), QStringLiteral("[SERVER]"), QStringLiteral("[MTP]"),
                                    QStringLiteral("[VISION]"), QStringLiteral("[MOE]")};

std::optional<LogEvent::Kind> appLineKind(const QString &line) {
    if (line.startsWith(QStringLiteral("[PLANNER ERROR]"))
        || line.startsWith(QStringLiteral("[CRITICAL RUNTIME ERROR]"))) {
        return LogEvent::Kind::Error;
    }
    const bool isWarning = line.contains(QStringLiteral("WARNING:"));
    if (line.startsWith(QStringLiteral("[PLANNER]"))) {
        return isWarning ? LogEvent::Kind::Warning : LogEvent::Kind::Planner;
    }
    for (const QString &prefix : LAUNCHER_PREFIXES) {
        if (line.startsWith(prefix)) {
            return isWarning ? LogEvent::Kind::Warning : LogEvent::Kind::Launcher;
        }
    }
    return std::nullopt;
}

} // namespace

LogEvent::Kind LogLineClassifier::classify(const QString &line) {
    if (const auto kind = appLineKind(line)) {
        return *kind;
    }
    // upstream and LM Studio: "M.SS.mmm.uuu L ..." where L is the level letter.
    static const QRegularExpression LEVELED(QStringLiteral("^\\d+\\.\\d{2}\\.\\d{3}\\.\\d{3} ([A-Z]) "));
    const QRegularExpressionMatch level = LEVELED.match(line);
    if (level.hasMatch()) {
        const QString letter = level.captured(1);
        if (letter == u'E') {
            return LogEvent::Kind::Error;
        }
        return letter == u'W' ? LogEvent::Kind::Warning : LogEvent::Kind::Server;
    }
    if (line.startsWith(QStringLiteral("LMSTUDIO_STARTUP_ERROR"))) {
        return LogEvent::Kind::Error;
    }
    static const QRegularExpression ERROR_WORDS(QStringLiteral("\\b(error|failed)\\b"),
                                                QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression WARNING_WORDS(QStringLiteral("^warning\\b|\\bwarning:"),
                                                  QRegularExpression::CaseInsensitiveOption);
    if (ERROR_WORDS.match(line).hasMatch()) {
        return LogEvent::Kind::Error;
    }
    return WARNING_WORDS.match(line).hasMatch() ? LogEvent::Kind::Warning : LogEvent::Kind::Server;
}

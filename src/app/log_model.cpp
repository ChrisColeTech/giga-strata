#include "log_model.h"

#include "launch/log_line_classifier.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QJSEngine>

LogModel::LogModel(QObject *parent) : QAbstractListModel(parent) {}

void LogModel::provide(LogModel *instance) {
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

LogModel *LogModel::create(QQmlEngine *, QJSEngine *) {
    Q_ASSERT_X(s_instance, "LogModel::create", "main.cpp must call LogModel::provide() before loading QML");
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

int LogModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : lineCount();
}

QVariant LogModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= lineCount()) {
        return {};
    }
    const Line &line = lineAt(index.row());
    switch (role) {
    case TextRole:
        return line.text;
    case KindRole:
        return kindName(line.kind);
    case TimeRole:
        return line.time.toString(QStringLiteral("hh:mm:ss"));
    default:
        return {};
    }
}

QHash<int, QByteArray> LogModel::roleNames() const {
    return {{TextRole, "text"}, {KindRole, "kind"}, {TimeRole, "time"}};
}

void LogModel::setFilter(const QString &filter) {
    if (filter == m_filter) {
        return;
    }
    m_filter = filter;
    beginResetModel();
    rebuildVisible();
    endResetModel();
    emit filterChanged();
    emit lineCountChanged();
}

void LogModel::appendMessage(const QString &message) {
    QStringList lines = message.split(u'\n');
    if (lines.size() > 1 && lines.constLast().isEmpty()) {
        lines.removeLast();
    }
    for (const QString &line : lines) {
        appendLine(line, LogLineClassifier::classify(line));
    }
}

void LogModel::appendLine(const QString &text, LogEvent::Kind kind) {
    // llama-server ends lines with CRLF; the stray halves arrive as empty lines and would put a
    // bare timestamp between every real line.
    if (text.trimmed().isEmpty()) {
        return;
    }
    if (m_lines.size() >= size_t(MAX_LINES)) {
        dropOldest();
    }
    m_lines.push_back({text, kind, QTime::currentTime()});
    if (!isShown(kind)) {
        return;
    }
    const int row = lineCount();
    beginInsertRows(QModelIndex(), row, row);
    m_visible.append(m_firstSequence + qint64(m_lines.size()) - 1);
    endInsertRows();
    emit lineCountChanged();
}

QStringList LogModel::visibleText(const QVariantList &rows) const {
    QStringList text;
    if (rows.isEmpty()) {
        for (int row = 0; row < lineCount(); ++row) {
            text.append(lineAt(row).text);
        }
        return text;
    }
    for (const QVariant &row : rows) {
        const int at = row.toInt();
        if (at >= 0 && at < lineCount()) {
            text.append(lineAt(at).text);
        }
    }
    return text;
}

void LogModel::copyAll() const {
    QGuiApplication::clipboard()->setText(visibleText().join(u'\n'));
}

void LogModel::copyLines(const QVariantList &rows) const {
    if (!rows.isEmpty()) {
        QGuiApplication::clipboard()->setText(visibleText(rows).join(u'\n'));
    }
}

void LogModel::clear() {
    beginResetModel();
    m_firstSequence += qint64(m_lines.size());
    m_lines.clear();
    m_visible.clear();
    endResetModel();
    emit lineCountChanged();
}

QString LogModel::kindName(LogEvent::Kind kind) {
    switch (kind) {
    case LogEvent::Kind::Planner:
        return QStringLiteral("planner");
    case LogEvent::Kind::Launcher:
        return QStringLiteral("launcher");
    case LogEvent::Kind::Warning:
        return QStringLiteral("warning");
    case LogEvent::Kind::Error:
        return QStringLiteral("error");
    case LogEvent::Kind::Server:
        break;
    }
    return QStringLiteral("server");
}

bool LogModel::isShown(LogEvent::Kind kind) const {
    if (m_filter == QLatin1String("server")) {
        return kind == LogEvent::Kind::Server;
    }
    if (m_filter == QLatin1String("planner")) {
        return kind == LogEvent::Kind::Planner || kind == LogEvent::Kind::Launcher;
    }
    if (m_filter == QLatin1String("warnings")) {
        return kind == LogEvent::Kind::Warning || kind == LogEvent::Kind::Error;
    }
    return true;
}

const LogModel::Line &LogModel::lineAt(int row) const {
    return m_lines[size_t(m_visible[row] - m_firstSequence)];
}

void LogModel::rebuildVisible() {
    m_visible.clear();
    for (size_t at = 0; at < m_lines.size(); ++at) {
        if (isShown(m_lines[at].kind)) {
            m_visible.append(m_firstSequence + qint64(at));
        }
    }
}

void LogModel::dropOldest() {
    if (!m_visible.isEmpty() && m_visible.constFirst() == m_firstSequence) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_visible.removeFirst();
        endRemoveRows();
        emit lineCountChanged();
    }
    m_lines.pop_front();
    ++m_firstSequence;
}

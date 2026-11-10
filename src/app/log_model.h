#pragma once

#include "launch/log_event.h"

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QVariantList>
#include <QVector>
#include <QtQml/qqmlregistration.h>

#include <deque>

class QJSEngine;
class QQmlEngine;

// The log drawer (05 section 5): launcher, planner and server lines with their kind (01 C3) and
// arrival time, filtered by `filter`. Keeps the last MAX_LINES lines; the oldest drop off.
class LogModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY lineCountChanged)

public:
    enum Role { TextRole = Qt::UserRole + 1, KindRole, TimeRole };
    static constexpr int MAX_LINES = 20000;

    explicit LogModel(QObject *parent);

    static void provide(LogModel *instance);
    static LogModel *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QString &filter() const { return m_filter; }
    // "all", "server", "planner" (planner and launcher lines) or "warnings" (warnings and errors).
    void setFilter(const QString &filter);
    int lineCount() const { return int(m_visible.size()); }

    // A message as run_process logs it: one or more lines ending in "\n" (a leading "\n" is a
    // blank line, as in the Tk log); each line is classified on its own.
    void appendMessage(const QString &message);
    void appendLine(const QString &text, LogEvent::Kind kind);
    // The text of the visible rows, in order.
    QStringList visibleText(const QVariantList &rows = {}) const;

    Q_INVOKABLE void copyAll() const;
    Q_INVOKABLE void copyLines(const QVariantList &rows) const;
    Q_INVOKABLE void clear();

    static QString kindName(LogEvent::Kind kind);

signals:
    void filterChanged();
    void lineCountChanged();

private:
    struct Line {
        QString text;
        LogEvent::Kind kind;
        QTime time;
    };

    bool isShown(LogEvent::Kind kind) const;
    const Line &lineAt(int row) const;
    void rebuildVisible();
    void dropOldest();

    static inline LogModel *s_instance = nullptr;

    std::deque<Line> m_lines;
    qint64 m_firstSequence = 0;          // sequence number of m_lines.front()
    QVector<qint64> m_visible;           // sequence numbers of the rows the filter shows
    QString m_filter = QStringLiteral("all");
};

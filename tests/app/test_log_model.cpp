#include "log_model.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QtTest>

// Runs on the offscreen platform so the copy actions have a clipboard.
class TestLogModel : public QObject {
    Q_OBJECT

private slots:
    void messagesSplitIntoClassifiedLines() {
        LogModel log{nullptr};
        log.appendMessage(QStringLiteral("\n[CRITICAL RUNTIME ERROR]: no GPUs selected\n"));
        log.appendMessage(QStringLiteral("[PLANNER] Micro-batch: auto -> 2048\n"));
        // Blank lines are dropped: CRLF halves from llama-server would otherwise fill the drawer
        // with bare timestamps.
        QCOMPARE(log.lineCount(), 2);
        const auto at = [&log](int row, int role) { return log.data(log.index(row), role).toString(); };
        QCOMPARE(at(0, LogModel::KindRole), QStringLiteral("error"));
        QCOMPARE(at(1, LogModel::KindRole), QStringLiteral("planner"));
        QCOMPARE(at(1, LogModel::TimeRole).size(), 8);
    }

    void filterShowsOnlyItsKinds() {
        LogModel log{nullptr};
        log.appendLine(QStringLiteral("server line"), LogEvent::Kind::Server);
        log.appendLine(QStringLiteral("[LAUNCHER] x"), LogEvent::Kind::Launcher);
        log.appendLine(QStringLiteral("[PLANNER] y"), LogEvent::Kind::Planner);
        log.appendLine(QStringLiteral("boom"), LogEvent::Kind::Error);
        log.appendLine(QStringLiteral("careful"), LogEvent::Kind::Warning);
        QSignalSpy countSpy(&log, &LogModel::lineCountChanged);
        log.setFilter(QStringLiteral("planner"));
        QCOMPARE(log.visibleText(), QStringList({QStringLiteral("[LAUNCHER] x"), QStringLiteral("[PLANNER] y")}));
        log.setFilter(QStringLiteral("warnings"));
        QCOMPARE(log.visibleText(), QStringList({QStringLiteral("boom"), QStringLiteral("careful")}));
        log.setFilter(QStringLiteral("server"));
        log.appendLine(QStringLiteral("[PLANNER] hidden"), LogEvent::Kind::Planner);
        QCOMPARE(log.lineCount(), 1);
        log.setFilter(QStringLiteral("all"));
        QCOMPARE(log.lineCount(), 6);
        QVERIFY(countSpy.count() >= 4);
    }

    void keepsOnlyTheNewestLines() {
        LogModel log{nullptr};
        for (int line = 0; line < LogModel::MAX_LINES + 5; ++line) {
            log.appendLine(QString::number(line), LogEvent::Kind::Server);
        }
        QCOMPARE(log.lineCount(), LogModel::MAX_LINES);
        QCOMPARE(log.data(log.index(0), LogModel::TextRole).toString(), QStringLiteral("5"));
        log.setFilter(QStringLiteral("server"));
        QCOMPARE(log.lineCount(), LogModel::MAX_LINES);
    }

    void copyAndClear() {
        LogModel log{nullptr};
        log.appendMessage(QStringLiteral("a\nb\nc\n"));
        log.copyLines({2, 0});
        QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("c\na"));
        log.copyAll();
        QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("a\nb\nc"));
        log.clear();
        QCOMPARE(log.lineCount(), 0);
        log.appendLine(QStringLiteral("after"), LogEvent::Kind::Server);
        QCOMPARE(log.data(log.index(0), LogModel::TextRole).toString(), QStringLiteral("after"));
    }
};

int main(int argc, char *argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", GIGA_STRATA_QT_PLATFORMS);
    QGuiApplication application(argc, argv);
    TestLogModel test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_log_model.moc"

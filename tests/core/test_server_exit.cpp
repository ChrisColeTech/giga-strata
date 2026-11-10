#include "launch/server_exit.h"

#include "golden_cases.h"
#include "launch/log_parser.h"

#include <QFile>
#include <QtTest>

namespace {

// Feeds a captured log and interprets its recorded exit code. Probes and serves were stopped by
// the capture tool (killed at "listening" / terminated), which is the app's stop request.
ServerOutcome outcomeOf(const QString &name) {
    const QString base = GoldenCases::root() + QStringLiteral("/fixtures/logs/") + name;
    QFile log(base + QStringLiteral(".log"));
    LogParser parser;
    if (log.open(QIODevice::ReadOnly)) {
        parser.feed(log.readAll());
        parser.finish();
    }
    const QJsonObject capture = GoldenCases::readObject(base + QStringLiteral(".json"));
    const QString notes = capture.value(QStringLiteral("notes")).toString();
    const bool wasStopped =
        notes.contains(QStringLiteral("killed at listening")) || notes.contains(QStringLiteral("Terminate"));
    return ServerExit::interpret(quint32(capture.value(QStringLiteral("exit_code")).toInteger()), wasStopped,
                                 parser.summary());
}

} // namespace

class TestServerExit : public QObject {
    Q_OBJECT

private slots:
    void finalStateOfEveryCapturedRun() {
        QCOMPARE(outcomeOf(QStringLiteral("missing_dll_lmstudio")).message, ServerExit::missingDllHint());
        QVERIFY(outcomeOf(QStringLiteral("missing_dll_lmstudio")).isFailure);
        const ServerOutcome crash = outcomeOf(QStringLiteral("probe_crash_upstream_mtp_sidecar"));
        QVERIFY(crash.isFailure);
        QVERIFY(crash.message.startsWith(QStringLiteral("The server crashed while loading the model")));
        QCOMPARE(outcomeOf(QStringLiteral("probe_fail_glm5_missing_tensor")).message,
                 QStringLiteral("The server could not load the model: missing tensor 'blk.0.attn_norm.weight'"));
        QCOMPARE(outcomeOf(QStringLiteral("bad_args_upstream_unknown_flag")).message,
                 QStringLiteral("The server rejected its arguments: invalid argument: --no-such-flag"));
        for (const char *stopped :
             {"probe_ok_upstream_qwen3-8b", "serve_cpu_glm5_qwen3-8b", "serve_cpu_lmstudio_qwen3-8b"}) {
            const ServerOutcome outcome = outcomeOf(QString::fromLatin1(stopped));
            QVERIFY2(!outcome.isFailure, stopped);
            QCOMPARE(outcome.message, QStringLiteral("Server stopped."));
        }
    }

    void unexpectedExitsAfterServing() {
        LogSummary serving;
        serving.hasStartedLoading = true;
        serving.isListening = true;
        QCOMPARE(ServerExit::interpret(0xC0000409, false, serving).message,
                 QStringLiteral("The server stopped unexpectedly (exit code 0xC0000409)."));
        QVERIFY(!ServerExit::interpret(0, false, serving).isFailure);
        QCOMPARE(ServerExit::interpret(1, false, LogSummary()).message,
                 QStringLiteral("The server exited with code 1 before it was ready."));
    }

    void launcherLinesMatchTheProcessScenarios() {
        for (const QString id : {QStringLiteral("t-proc-dll"), QStringLiteral("t-proc-lines")}) {
            const QString file = id + QStringLiteral(".json");
            const QJsonObject fixture =
                GoldenCases::readObject(GoldenCases::root() + QStringLiteral("/fixtures/scenarios/") + file);
            const QJsonObject expected =
                GoldenCases::readObject(GoldenCases::root() + QStringLiteral("/expected/scenarios/") + file);
            const QJsonObject process = fixture.value(QStringLiteral("process")).toObject();
            const auto code = quint32(process.value(QStringLiteral("exit_code")).toInteger());
            QStringList tail;
            const QJsonArray log = expected.value(QStringLiteral("log")).toArray();
            const QStringList lines = ServerExit::launcherLines(code);
            for (qsizetype at = log.size() - lines.size(); at < log.size(); ++at) {
                tail.append(log.at(at).toString());
            }
            QCOMPARE(lines, tail);
        }
    }
};

QTEST_APPLESS_MAIN(TestServerExit)
#include "test_server_exit.moc"

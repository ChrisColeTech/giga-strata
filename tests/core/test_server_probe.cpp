#include "launch/server_probe.h"

#include "launch/line_splitter.h"

#include <QFile>
#include <QtTest>

// The probe's process side needs a real llama-server, so only the verdict logic runs here: every
// captured log, fed through the same line splitter, must give what Python's probe_server loop
// gives on that file (computed with CPython on the fixtures).
namespace {

std::optional<ProbeVerdict> verdictOf(const QString &logName) {
    QFile log(QStringLiteral(GIGA_STRATA_TEST_DATA "/../../golden/fixtures/logs/") + logName);
    if (!log.open(QIODevice::ReadOnly)) {
        return ProbeVerdict{false, QStringLiteral("fixture missing")};
    }
    LineSplitter splitter;
    QStringList lines = splitter.feed(log.readAll());
    lines += splitter.finish();
    for (const QString &line : lines) {
        if (const auto verdict = ServerProbe::verdictForLine(line)) {
            return verdict;
        }
    }
    return std::nullopt;
}

} // namespace

class TestServerProbe : public QObject {
    Q_OBJECT

private slots:
    void verdictsMatchPythonOnEveryCapturedLog_data() {
        QTest::addColumn<QString>("log");
        QTest::addColumn<bool>("hasVerdict");
        QTest::addColumn<bool>("isLoadable");
        QTest::addColumn<QString>("reason");
        const auto none = [](const char *log) {
            QTest::newRow(log) << QString::fromLatin1(log) << false << false << QString();
        };
        const auto ok = [](const char *log) {
            QTest::newRow(log) << QString::fromLatin1(log) << true << true << QStringLiteral("ok");
        };
        const auto fail = [](const char *log, const QString &reason) {
            QTest::newRow(log) << QString::fromLatin1(log) << true << false << reason;
        };
        for (const char *log : {"bad_args_glm5_unknown_flag.log", "bad_args_lmstudio_ctx_abc.log",
                                "bad_args_upstream_ctx_abc.log", "bad_args_upstream_unknown_flag.log",
                                "missing_dll_lmstudio.log",
                                "probe_crash_upstream_mtp_sidecar.log"}) {
            none(log);
        }
        fail("probe_fail_glm5_invalid_ggml_type.log",
             QStringLiteral("llama_model_loader: failed to load model from N:\\strata\\model\\mtp-q2_0.gguf"));
        fail("probe_fail_glm5_missing_tensor.log", QStringLiteral("missing tensor 'blk.0.attn_norm.weight'"));
        fail("probe_fail_lmstudio_unknown_arch.log", QStringLiteral("unknown model architecture: 'qwen4exp-mtp'"));
        fail("probe_fail_upstream_unknown_arch.log", QStringLiteral("unknown model architecture: 'qwen4exp-mtp'"));
        for (const char *log : {"probe_ok_glm5_glm53-3bit.log", "probe_ok_glm5_qwen3-8b.log",
                                "probe_ok_lmstudio_glm53-3bit.log", "probe_ok_lmstudio_qwen3-8b.log",
                                "probe_ok_upstream_glm53-3bit.log", "probe_ok_upstream_qwen3-8b.log",
                                "probe_ok_upstream_qwen3-8b_lv4.log",
                                "serve_cpu_glm5_qwen3-8b.log", "serve_cpu_lmstudio_qwen3-8b.log",
                                "serve_cpu_upstream_qwen3-8b.log"}) {
            ok(log);
        }
    }

    void verdictsMatchPythonOnEveryCapturedLog() {
        QFETCH(QString, log);
        QFETCH(bool, hasVerdict);
        QFETCH(bool, isLoadable);
        QFETCH(QString, reason);
        const auto verdict = verdictOf(log);
        QCOMPARE(verdict.has_value(), hasVerdict);
        if (verdict) {
            QCOMPARE(verdict->isLoadable, isLoadable);
            QCOMPARE(verdict->reason, reason);
        }
    }

    void argumentsMatchProbeServer() {
        QCOMPARE(ServerProbe::arguments(QStringLiteral("m.gguf"), 5000).join(u' '),
                 QStringLiteral("-m m.gguf -ngl 0 -c 256 -np 1 --no-warmup --host 127.0.0.1 --port 5000"));
    }

    void splitterHandlesCrLfAcrossChunks() {
        LineSplitter splitter;
        QCOMPARE(splitter.feed("a\r"), QStringList({QStringLiteral("a")}));
        QCOMPARE(splitter.feed("\nb\rc\n..."), QStringList({QStringLiteral("b"), QStringLiteral("c")}));
        QCOMPARE(splitter.pending(), QByteArray("..."));
        QCOMPARE(splitter.finish(), QStringList({QStringLiteral("...")}));
    }
};

QTEST_APPLESS_MAIN(TestServerProbe)
#include "test_server_probe.moc"

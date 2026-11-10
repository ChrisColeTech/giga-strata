#include "launch/log_parser.h"

#include <QDir>
#include <QFile>
#include <QMap>
#include <QtTest>

// Expected event sequences were derived by reading each captured log against the line analysis
// in tests/golden/fixtures/logs/README.md (tok/s are the figures the server printed).
namespace {

const QString LOGS = QStringLiteral(GIGA_STRATA_TEST_DATA "/../../golden/fixtures/logs/");
const QString QWEN = QStringLiteral("D:\\models\\image-models\\hymotion\\Qwen3-8B-GGUF\\Qwen3-8B-Q4_K_M.gguf");
const QString GLM =
    QStringLiteral("N:\\PolyStrata-data\\models\\glm-3.0BIT\\GLM-5.3-Flash-GSQ-RCO-3.0bit-q4kattn.gguf");
const QString SIDECAR =
    QStringLiteral("N:\\PolyStrata-data\\models\\glm-3.0BIT\\GLM-5.3-Flash-GSQ-RCO-3.0bit-q4kattn-MTP-sidecar.gguf");
const QString MTP = QStringLiteral("N:\\strata\\model\\mtp-q2_0.gguf");
const QString UNKNOWN_ARCH = QStringLiteral("unknown model architecture: 'qwen4exp-mtp'");

QString describe(const LogEvent &event) {
    switch (event.type) {
    case LogEvent::Type::LoadingStarted: return QStringLiteral("loading:") + event.text;
    case LogEvent::Type::Listening: return QStringLiteral("listening:") + event.text;
    case LogEvent::Type::LoadError: return QStringLiteral("load_error:") + event.text;
    case LogEvent::Type::ArgumentError: return QStringLiteral("argument_error:") + event.text;
    case LogEvent::Type::StartupError: return QStringLiteral("startup_error:%1:%2").arg(event.code, event.text);
    case LogEvent::Type::RequestTiming:
        return QStringLiteral("timing:%1:%2:%3")
            .arg(event.isPrompt ? QStringLiteral("prompt") : QStringLiteral("eval"))
            .arg(event.taskId)
            .arg(event.tokensPerSecondText);
    default: return QString();
    }
}

QByteArray readLog(const QString &name) {
    QFile file(LOGS + name);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

QVector<LogEvent> parseInChunks(const QByteArray &bytes, qsizetype chunk, LogParser &parser) {
    QVector<LogEvent> events;
    const qsizetype step = chunk > 0 ? chunk : std::max<qsizetype>(bytes.size(), 1);
    for (qsizetype at = 0; at < bytes.size(); at += step) {
        events += parser.feed(bytes.mid(at, step));
    }
    return events + parser.finish();
}

QStringList timings(const QString &task, const QString &prompt, const QString &eval) {
    return {QStringLiteral("timing:prompt:") + task + u':' + prompt,
            QStringLiteral("timing:eval:") + task + u':' + eval};
}

QMap<QString, QStringList> expectedEvents() {
    const QString loadQwen = QStringLiteral("loading:") + QWEN;
    const QString loadGlm = QStringLiteral("loading:") + GLM;
    const QString loadMtp = QStringLiteral("loading:") + MTP;
    const auto listen = [](const char *port) {
        return QStringLiteral("listening:http://127.0.0.1:") + QLatin1String(port);
    };
    const QString badValue =
        QStringLiteral("argument_error:error while handling argument \"-c\": invalid stoi argument");
    const QString badFlag = QStringLiteral("argument_error:invalid argument: --no-such-flag");
    return {
        {QStringLiteral("bad_args_glm5_unknown_flag"), {badFlag}},
        {QStringLiteral("bad_args_lmstudio_ctx_abc"), {badValue}},
        {QStringLiteral("bad_args_upstream_ctx_abc"), {badValue}},
        {QStringLiteral("bad_args_upstream_unknown_flag"), {badFlag}},
        {QStringLiteral("missing_dll_lmstudio"), {}},
        {QStringLiteral("probe_crash_upstream_mtp_sidecar"), {QStringLiteral("loading:") + SIDECAR}},
        {QStringLiteral("probe_fail_glm5_invalid_ggml_type"),
         {loadMtp, QStringLiteral("load_error:tensor 'mtp.layers.0.mlp.experts.down_proj' has invalid ggml type 42. "
                                  "should be in [0, 41)")}},
        {QStringLiteral("probe_fail_glm5_missing_tensor"),
         {QStringLiteral("loading:") + SIDECAR, QStringLiteral("load_error:missing tensor 'blk.0.attn_norm.weight'")}},
        {QStringLiteral("probe_fail_lmstudio_unknown_arch"),
         {loadMtp, QStringLiteral("load_error:") + UNKNOWN_ARCH,
          QStringLiteral("startup_error:model_load_failed:error loading model: ") + UNKNOWN_ARCH}},
        {QStringLiteral("probe_fail_upstream_unknown_arch"), {loadMtp, QStringLiteral("load_error:") + UNKNOWN_ARCH}},
        {QStringLiteral("probe_ok_glm5_glm53-3bit"), {loadGlm, listen("51109")}},
        {QStringLiteral("probe_ok_glm5_qwen3-8b"), {loadQwen, listen("49280")}},
        {QStringLiteral("probe_ok_lmstudio_glm53-3bit"), {loadGlm, listen("49768")}},
        {QStringLiteral("probe_ok_lmstudio_qwen3-8b"), {loadQwen, listen("49335")}},
        {QStringLiteral("probe_ok_upstream_glm53-3bit"), {loadGlm, listen("49338")}},
        {QStringLiteral("probe_ok_upstream_qwen3-8b"), {loadQwen, listen("49265")}},
        {QStringLiteral("probe_ok_upstream_qwen3-8b_lv4"), {loadQwen, listen("51056")}},
        {QStringLiteral("serve_cpu_glm5_qwen3-8b"),
         QStringList{loadQwen, listen("50819")}
             + timings(QStringLiteral("0"), QStringLiteral("46.81"), QStringLiteral("10.28"))
             + timings(QStringLiteral("65"), QStringLiteral("65.09"), QStringLiteral("10.32"))},
        {QStringLiteral("serve_cpu_lmstudio_qwen3-8b"),
         QStringList{loadQwen, listen("50851")}
             + timings(QStringLiteral("0"), QStringLiteral("50.37"), QStringLiteral("10.36"))
             + timings(QStringLiteral("65"), QStringLiteral("73.23"), QStringLiteral("10.31"))},
        {QStringLiteral("serve_cpu_upstream_qwen3-8b"),
         QStringList{loadQwen, listen("50582")}
             + timings(QStringLiteral("0"), QStringLiteral("43.35"), QStringLiteral("9.93"))
             + timings(QStringLiteral("65"), QStringLiteral("65.71"), QStringLiteral("10.06"))},
    };
}

} // namespace

class TestLogParser : public QObject {
    Q_OBJECT

private slots:
    void everyCapturedLogGivesItsEventsInAnyChunking() {
        const QMap<QString, QStringList> expected = expectedEvents();
        QCOMPARE(QDir(LOGS).entryList({QStringLiteral("*.log")}).size(), expected.size());
        for (auto log = expected.cbegin(); log != expected.cend(); ++log) {
            const QByteArray bytes = readLog(log.key() + QStringLiteral(".log"));
            QVERIFY2(!bytes.isEmpty() || log.key().startsWith(QStringLiteral("missing_dll")), qPrintable(log.key()));
            for (const qsizetype chunk : {qsizetype(0), qsizetype(1), qsizetype(7), qsizetype(4096)}) {
                LogParser parser;
                QStringList described;
                int lines = 0;
                int lastProgress = -1;
                for (const LogEvent &event : parseInChunks(bytes, chunk, parser)) {
                    lines += event.type == LogEvent::Type::Line ? 1 : 0;
                    if (event.type == LogEvent::Type::LoadProgress) {
                        QVERIFY(event.progressPercent > lastProgress);
                        lastProgress = event.progressPercent;
                    } else if (event.type != LogEvent::Type::Line) {
                        described.append(describe(event));
                    }
                }
                QCOMPARE(described, log.value());
                QCOMPARE(lines, int(bytes.count('\n')));
                // Only glm5 prints progress dots, and only a load that gets through prints them all.
                const bool isGlm5Load = log.key().contains(QStringLiteral("_glm5_"))
                                        && (log.key().startsWith(QStringLiteral("probe_ok"))
                                            || log.key().startsWith(QStringLiteral("serve")));
                QCOMPARE(lastProgress, isGlm5Load ? 100 : -1);
            }
        }
    }

    void dotsReportLiveProgressBeforeTheirNewline() {
        LogParser parser;
        QVERIFY(parser.feed("load_tensors:   CPU_REPACK model buffer size =  3199.50 MiB\r\n..").size() == 2);
        const QVector<LogEvent> more = parser.feed("...");
        QCOMPARE(more.size(), 1);
        QCOMPARE(more.first().progressPercent, 5);
        const QVector<LogEvent> done = parser.feed("\r\n");
        QCOMPARE(done.last().progressPercent, 100);
    }

    void summaryTracksTheRun() {
        LogParser parser;
        parser.feed(readLog(QStringLiteral("probe_fail_lmstudio_unknown_arch.log")));
        QVERIFY(parser.summary().hasStartedLoading);
        QVERIFY(!parser.summary().isListening);
        QCOMPARE(parser.summary().loadError, UNKNOWN_ARCH);
        QVERIFY(parser.summary().lastErrorLine.contains(QStringLiteral("exiting due to model loading error")));
    }
};

QTEST_APPLESS_MAIN(TestLogParser)
#include "test_log_parser.moc"

#include "launch/log_line_classifier.h"

#include <QtTest>

class TestLogLineClassifier : public QObject {
    Q_OBJECT

private slots:
    void colorsLinesLikeTheStyleGuide_data() {
        QTest::addColumn<QString>("line");
        QTest::addColumn<int>("kind");
        const auto row = [](const char *line, LogEvent::Kind kind) {
            QTest::newRow(line) << QString::fromUtf8(line) << int(kind);
        };
        row("[PLANNER] Model: glm5-next, 45 layers", LogEvent::Kind::Planner);
        row("[PLANNER] WARNING: RAM tier (90.1 GiB) does not fit", LogEvent::Kind::Warning);
        row("[PLANNER ERROR]: no GPUs selected", LogEvent::Kind::Error);
        row("[CRITICAL RUNTIME ERROR]: boom", LogEvent::Kind::Error);
        row("[LAUNCHER] Executing command: x", LogEvent::Kind::Launcher);
        row("[MOE] WARNING: under 20,000 generated tokens recorded", LogEvent::Kind::Warning);
        row("[SERVER] Trial-loading the model with x ...", LogEvent::Kind::Launcher);
        row("0.00.057.812 E llama_model_load: error loading model: x", LogEvent::Kind::Error);
        row("0.00.052.257 W srv  server_setup: security: no API key is set", LogEvent::Kind::Warning);
        row("0.03.543.934 I srv  server_on_li: listening on http://127.0.0.1:1", LogEvent::Kind::Server);
        row("0.00.058.102 I srv    operator(): operator(): cleaning up before exit...", LogEvent::Kind::Server);
        row("warning: no usable GPU found, --gpu-layers option will be ignored", LogEvent::Kind::Warning);
        row("error: invalid argument: --no-such-flag", LogEvent::Kind::Error);
        row("LMSTUDIO_STARTUP_ERROR:{\"code\":\"model_load_failed\"}", LogEvent::Kind::Error);
        row("llama_model_load_from_file_impl: failed to load model", LogEvent::Kind::Error);
        row("main: exiting due to model loading error", LogEvent::Kind::Error);
        row("main: server is listening on http://127.0.0.1:50819", LogEvent::Kind::Server);
        row("       eval time =    6226.90 ms /    64 tokens", LogEvent::Kind::Server);
        row("llama_params_fit: successfully fit params to free device memory", LogEvent::Kind::Server);
    }

    void colorsLinesLikeTheStyleGuide() {
        QFETCH(QString, line);
        QFETCH(int, kind);
        QCOMPARE(int(LogLineClassifier::classify(line)), kind);
    }
};

QTEST_APPLESS_MAIN(TestLogLineClassifier)
#include "test_log_line_classifier.moc"

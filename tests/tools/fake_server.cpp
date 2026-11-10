#include <QByteArray>
#include <QFile>
#include <QThread>

#include <cstdio>
#include <cstring>

#include <windows.h>

// Stands in for llama-server in LaunchController tests, so no test ever loads a model or touches
// a GPU. `--help` prints every optional flag the launcher looks for. Otherwise it records its raw
// command line to GIGA_STRATA_FAKE_ARGS (if set), prints the captured log GIGA_STRATA_FAKE_LOG
// (tests/golden/fixtures/logs) line by line, then either waits to be killed
// (GIGA_STRATA_FAKE_HOLD=1) or exits with GIGA_STRATA_FAKE_EXIT (default 0).
namespace {

constexpr unsigned long LINE_DELAY_MS = 2;

void printHelp() {
    std::fputs("--fit --load-mode --override-tensor --lazy-mode --backend-sampling --spec-type draft-mtp,ngram-mod "
               "--moe-hot --alias --mmproj-device\n",
               stdout);
}

void recordCommandLine() {
    const QByteArray path = qgetenv("GIGA_STRATA_FAKE_ARGS");
    QFile out(QString::fromLocal8Bit(path));
    if (!path.isEmpty() && out.open(QIODevice::WriteOnly)) {
        out.write(QString::fromWCharArray(GetCommandLineW()).toUtf8());
    }
}

void printLog() {
    QFile log(QString::fromLocal8Bit(qgetenv("GIGA_STRATA_FAKE_LOG")));
    if (!log.open(QIODevice::ReadOnly)) {
        return;
    }
    while (!log.atEnd()) {
        const QByteArray line = log.readLine();
        std::fwrite(line.constData(), 1, size_t(line.size()), stdout);
        std::fflush(stdout);
        QThread::msleep(LINE_DELAY_MS);
    }
}

} // namespace

int main(int argc, char *argv[]) {
    for (int at = 1; at < argc; ++at) {
        if (std::strcmp(argv[at], "--help") == 0) {
            printHelp();
            return 0;
        }
    }
    recordCommandLine();
    printLog();
    if (qgetenv("GIGA_STRATA_FAKE_HOLD") == "1") {
        for (;;) {
            QThread::sleep(1);
        }
    }
    return int(qgetenv("GIGA_STRATA_FAKE_EXIT").toUInt());
}

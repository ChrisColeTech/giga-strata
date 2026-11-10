#include "launch/extra_args.h"
#include "launch/flag_probe.h"
#include "launch/local_server_environment.h"
#include "launch/log_parser.h"
#include "launch/server_env.h"
#include "launch/server_finder.h"
#include "launch/server_probe.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

// Parity tool for core/launch (02 section 8) on the real machine: one JSON request per stdin line
// ({"op": "shlex", "text"} | {"op": "find_servers", "root"} | {"op": "flags", "binary"} |
// {"op": "probe", "binary", "model", "timeout", "env"}), one
// JSON answer per line, diffed against llama_launcher.py by an out-of-repo Python harness.
namespace {

QJsonArray sorted(const QSet<QString> &values) {
    QStringList list(values.cbegin(), values.cend());
    list.sort();
    return QJsonArray::fromStringList(list);
}

// Every non-line event a log file produces when fed in chunks of `chunkSize` bytes (0 = whole).
QJsonObject logEvents(const QString &path, int chunkSize) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {{QStringLiteral("error"), path}};
    }
    const QByteArray bytes = file.readAll();
    LogParser parser;
    QVector<LogEvent> events;
    const qsizetype step = chunkSize > 0 ? chunkSize : std::max<qsizetype>(bytes.size(), 1);
    for (qsizetype at = 0; at < bytes.size(); at += step) {
        events += parser.feed(bytes.mid(at, step));
    }
    events += parser.finish();
    QJsonArray out;
    QJsonArray kinds;
    for (const LogEvent &event : events) {
        if (event.type == LogEvent::Type::Line) {
            kinds.append(int(event.kind));
            continue;
        }
        out.append(QJsonObject{{QStringLiteral("type"), int(event.type)}, {QStringLiteral("text"), event.text},
                               {QStringLiteral("progress"), event.progressPercent},
                               {QStringLiteral("prompt"), event.isPrompt}, {QStringLiteral("task"), event.taskId},
                               {QStringLiteral("tps"), event.tokensPerSecondText},
                               {QStringLiteral("code"), event.code}});
    }
    return {{QStringLiteral("events"), out}, {QStringLiteral("kinds"), kinds}};
}

QJsonObject answer(const QJsonObject &request, const LocalServerEnvironment &environment, FlagProbe &probe) {
    const QString op = request.value(QStringLiteral("op")).toString();
    if (op == QStringLiteral("shlex")) {
        const auto tokens = ExtraArgs::split(request.value(QStringLiteral("text")).toString());
        if (!tokens.ok()) {
            return {{QStringLiteral("error"), tokens.error()}};
        }
        return {{QStringLiteral("tokens"), QJsonArray::fromStringList(tokens.value())}};
    }
    if (op == QStringLiteral("find_servers")) {
        const QString root = request.value(QStringLiteral("root")).toString();
        return {{QStringLiteral("servers"),
                 QJsonArray::fromStringList(ServerFinder::find(environment, ServerFinder::upstreamServer(root),
                                                               ServerFinder::builtServer(root)))},
                {QStringLiteral("default"), ServerFinder::defaultBinary(environment, root)}};
    }
    if (op == QStringLiteral("log")) {
        const QString path = request.value(QStringLiteral("path")).toString();
        return logEvents(path, request.value(QStringLiteral("chunk")).toInt());
    }
    const QString binary = request.value(QStringLiteral("binary")).toString();
    if (op == QStringLiteral("probe")) {
        QProcessEnvironment probeEnvironment = ServerEnv::forProbe(QProcessEnvironment::systemEnvironment(), {});
        const QJsonObject extra = request.value(QStringLiteral("env")).toObject();
        for (auto key = extra.begin(); key != extra.end(); ++key) {
            probeEnvironment.insert(key.key(), key.value().toString());
        }
        QElapsedTimer timer;
        timer.start();
        const QString model = request.value(QStringLiteral("model")).toString();
        const int timeout = request.value(QStringLiteral("timeout")).toInt();
        const ProbeVerdict verdict = ServerProbe::probe(binary, model, probeEnvironment, timeout);
        return {{QStringLiteral("ok"), verdict.isLoadable},
                {QStringLiteral("reason"), verdict.reason},
                {QStringLiteral("ms"), timer.elapsed()}};
    }
    const QStringList vendor = ServerEnv::vendorDllDirectories(environment, binary);
    const QProcessEnvironment processEnvironment =
        ServerEnv::build(QProcessEnvironment::systemEnvironment(), vendor, {});
    return {{QStringLiteral("flags"), sorted(probe.flags(binary, processEnvironment))},
            {QStringLiteral("vendor"), QJsonArray::fromStringList(vendor)}};
}

} // namespace

int main(int argc, char *argv[]) {
    QCoreApplication application(argc, argv);
    QFile input;
    if (!input.open(stdin, QIODevice::ReadOnly)) {
        return 1;
    }
    QTextStream out(stdout);
    out.setEncoding(QStringConverter::Utf8);
    const LocalServerEnvironment environment;
    FlagProbe probe(environment);
    while (!input.atEnd()) {
        const QByteArray line = input.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const QJsonObject request = QJsonDocument::fromJson(line).object();
        out << QJsonDocument(answer(request, environment, probe)).toJson(QJsonDocument::Compact) << '\n';
        out.flush();
    }
    return 0;
}

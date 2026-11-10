#include "launch/server_finder.h"

#include "fake_server_environment.h"
#include "golden_cases.h"

#include <QtTest>

namespace {

QStringList strings(const QJsonValue &value) {
    QStringList out;
    for (const QJsonValue &item : value.toArray()) {
        out.append(item.toString());
    }
    return out;
}

} // namespace

class TestServerFinder : public QObject {
    Q_OBJECT

private slots:
    void findsTheSameListAsTheReference() {
        const auto cases = GoldenCases::small(QStringLiteral("find_servers"));
        QVERIFY(cases.size() >= 5);
        for (const auto &[input, expected] : cases) {
            FakeServerEnvironment environment;
            environment.home = input.value(QStringLiteral("home")).toString();
            environment.pathDirectories = strings(input.value(QStringLiteral("path_dirs")));
            // The glob's results come in NTFS order; list those folders first so the fake
            // directory listing has that order too.
            QStringList files;
            for (const QJsonValue &call : input.value(QStringLiteral("glob_calls")).toArray()) {
                files += strings(call.toObject().value(QStringLiteral("results")));
            }
            for (const QString &file : strings(input.value(QStringLiteral("files")))) {
                if (!files.contains(file)) {
                    files.append(file);
                }
            }
            for (const QString &file : files) {
                environment.addFile(file);
            }
            const QStringList servers =
                ServerFinder::find(environment, input.value(QStringLiteral("upstream_server")).toString(),
                                   input.value(QStringLiteral("built_server")).toString());
            QCOMPARE(servers, strings(expected.value(QStringLiteral("servers"))));
        }
    }

    void versionKeyUsesPythonInt() {
        const auto cases = GoldenCases::small(QStringLiteral("version_key"));
        QVERIFY(cases.size() >= 10);
        for (const auto &[input, expected] : cases) {
            QVector<qint64> key;
            for (const QJsonValue &part : expected.value(QStringLiteral("key")).toArray()) {
                key.append(part.toInteger());
            }
            QCOMPARE(ServerFinder::versionKey(input.value(QStringLiteral("path")).toString()), key);
        }
    }

    void defaultBinaryPrefersUpstream() {
        FakeServerEnvironment environment;
        QCOMPARE(ServerFinder::defaultBinary(environment, QStringLiteral("C:\\p")), QStringLiteral("llama-server"));
        environment.addFile(ServerFinder::builtServer(QStringLiteral("C:\\p")));
        QCOMPARE(ServerFinder::defaultBinary(environment, QStringLiteral("C:\\p")),
                 QStringLiteral("C:\\p\\third_party\\llama.cpp-glm5\\bin\\bin\\Release\\llama-server.exe"));
        environment.addFile(ServerFinder::upstreamServer(QStringLiteral("C:\\p")));
        QCOMPARE(ServerFinder::defaultBinary(environment, QStringLiteral("C:\\p")),
                 ServerFinder::upstreamServer(QStringLiteral("C:\\p")));
    }
};

QTEST_APPLESS_MAIN(TestServerFinder)
#include "test_server_finder.moc"

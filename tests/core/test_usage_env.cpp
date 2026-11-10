#include "launch/usage_env.h"

#include "common/model_names.h"
#include "gguf/model_layout.h"
#include "golden_cases.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

namespace {

ModelLayout layoutFor(const QString &modelPath) {
    const QJsonArray models =
        GoldenCases::readObject(GoldenCases::root() + QStringLiteral("/fixtures/models/index.json"))
            .value(QStringLiteral("models"))
            .toArray();
    for (const QJsonValue &model : models) {
        if (model.toObject().value(QStringLiteral("original_path")).toString() != modelPath) {
            continue;
        }
        const QJsonObject json = GoldenCases::readObject(GoldenCases::root() + QStringLiteral("/expected/layout/") +
                                                         model.toObject().value(QStringLiteral("id")).toString() +
                                                         QStringLiteral(".json"))
                                     .value(QStringLiteral("layout"))
                                     .toObject();
        ModelLayout layout;
        layout.arch = json.value(QStringLiteral("arch")).toString();
        layout.layerCount = json.value(QStringLiteral("n_layer")).toInt();
        layout.skippedLayerCount = json.value(QStringLiteral("n_skipped")).toInt();
        layout.expertCount = json.value(QStringLiteral("n_expert")).toInt();
        return layout;
    }
    return {};
}

std::optional<QByteArray> fileBefore(const QJsonObject &fixture, const QString &path) {
    for (const QJsonValue &value : fixture.value(QStringLiteral("files_before")).toArray()) {
        const QJsonObject file = value.toObject();
        if (file.value(QStringLiteral("path")).toString().compare(path, Qt::CaseInsensitive) != 0) {
            continue;
        }
        if (file.contains(QStringLiteral("text"))) {
            return file.value(QStringLiteral("text")).toString().toUtf8();
        }
        QFile profile(GoldenCases::root() + QStringLiteral("/fixtures/usage/") +
                      file.value(QStringLiteral("usage_profile")).toString() + QStringLiteral(".usage.json"));
        return profile.open(QIODevice::ReadOnly) ? std::optional<QByteArray>(profile.readAll()) : std::nullopt;
    }
    return std::nullopt;
}

bool isUsageEnvLine(const QString &line) {
    return line.startsWith(QStringLiteral("[MOE] Not recording")) ||
           line.startsWith(QStringLiteral("[MOE] WARNING: this llama-server build predates")) ||
           line.contains(QStringLiteral(" was not recorded with this model;")) ||
           line.startsWith(QStringLiteral("[MOE] Recording expert usage"));
}

} // namespace

class TestUsageEnv : public QObject {
    Q_OBJECT

private slots:
    void matchesEveryRecordingScenario() {
        const QString root = GoldenCases::root();
        const QJsonArray ids = GoldenCases::readObject(root + QStringLiteral("/fixtures/scenarios/index.json"))
                                   .value(QStringLiteral("scenarios"))
                                   .toArray();
        int checked = 0;
        for (const QJsonValue &id : ids) {
            const QString name = id.toString() + QStringLiteral(".json");
            const QJsonObject fixture = GoldenCases::readObject(root + QStringLiteral("/fixtures/scenarios/") + name);
            const QJsonObject expected = GoldenCases::readObject(root + QStringLiteral("/expected/scenarios/") + name);
            const QJsonObject settings = fixture.value(QStringLiteral("settings")).toObject();
            const bool isLaunched = !expected.value(QStringLiteral("argv")).isNull();
            if (!settings.value(QStringLiteral("record_usage")).toBool() || !isLaunched) {
                continue;
            }
            const QString model = settings.value(QStringLiteral("model")).toString();
            const ModelLayout layout = layoutFor(model);
            const QString usageDirectory = fixture.value(QStringLiteral("usage_dir")).toString();
            const QString usagePath = ModelNames::usagePath(usageDirectory, model);
            const auto contents = fileBefore(fixture, usagePath);
            QSet<QString> flags;
            for (const QJsonValue &flag : fixture.value(QStringLiteral("flags")).toArray()) {
                flags.insert(flag.toString());
            }
            QProcessEnvironment environment;
            const QJsonObject base = fixture.value(QStringLiteral("env_base")).toObject();
            for (auto key = base.begin(); key != base.end(); ++key) {
                environment.insert(key.key(), key.value().toString());
            }
            const UsageRecording recording =
                UsageEnv::plan(layout, flags, usagePath, UsageEnv::isCompatible(contents.has_value(), contents, layout),
                               environment);
            QStringList expectedLines;
            for (const QJsonValue &line : expected.value(QStringLiteral("log")).toArray()) {
                if (isUsageEnvLine(line.toString())) {
                    expectedLines.append(line.toString());
                }
            }
            QCOMPARE(recording.logLines, expectedLines);
            UsageEnv::apply(recording, environment);
            const QJsonObject delta = expected.value(QStringLiteral("env_delta")).toObject();
            for (const QString key : {QStringLiteral("LLAMA_MOE_USAGE_OUT"), QStringLiteral("LLAMA_MOE_USAGE_FLUSH")}) {
                if (delta.contains(key)) {
                    QCOMPARE(environment.value(key), delta.value(key).toString());
                }
            }
            ++checked;
        }
        QVERIFY(checked >= 50);
    }

    void prepareMovesAForeignFileAside() {
        QTemporaryDir root;
        const QString directory = QDir::toNativeSeparators(root.filePath(QStringLiteral("moe_usage")));
        const QString path = directory + QStringLiteral("\\m.usage.json");
        QVERIFY(QDir().mkpath(directory));
        QFile foreign(path);
        QVERIFY(foreign.open(QIODevice::WriteOnly));
        const QByteArray foreignBytes = R"({"arch":"other","n_expert":8,"n_layer":4})";
        foreign.write(foreignBytes);
        foreign.close();
        QFile stale(path + QStringLiteral(".bak"));
        QVERIFY(stale.open(QIODevice::WriteOnly));
        stale.close();
        ModelLayout layout;
        layout.arch = QStringLiteral("glm5-next");
        layout.expertCount = 8;
        layout.layerCount = 4;
        QProcessEnvironment environment;
        const auto recording = UsageEnv::prepare(layout, {}, directory, path, environment);
        QVERIFY(recording.ok());
        QVERIFY(recording.value().needsBackup);
        QVERIFY(!QFile::exists(path));
        QCOMPARE(QFileInfo(path + QStringLiteral(".bak")).size(), qint64(foreignBytes.size()));
        QCOMPARE(environment.value(QStringLiteral("LLAMA_MOE_USAGE_OUT")), path);
        QCOMPARE(environment.value(QStringLiteral("LLAMA_MOE_USAGE_FLUSH")), QStringLiteral("64"));
    }
};

QTEST_APPLESS_MAIN(TestUsageEnv)
#include "test_usage_env.moc"

#include "launch/server_env.h"

#include "fake_server_environment.h"
#include "golden_cases.h"

#include <QtTest>

class TestServerEnv : public QObject {
    Q_OBJECT

private slots:
    void vendorDirectoriesAndEnvironmentMatchTheReference() {
        const auto cases = GoldenCases::small(QStringLiteral("vendor_dll_dirs"));
        QVERIFY(cases.size() >= 8);
        for (const auto &[input, expected] : cases) {
            FakeServerEnvironment environment;
            const QJsonObject directories = input.value(QStringLiteral("dirs")).toObject();
            for (auto directory = directories.begin(); directory != directories.end(); ++directory) {
                environment.addDirectory(directory.key());
                for (const QJsonValue &entry : directory.value().toArray()) {
                    environment.addDirectory(directory.key() + u'\\' + entry.toString());
                }
            }
            const QString binary = input.value(QStringLiteral("binary")).toString();
            const QStringList vendor = ServerEnv::vendorDllDirectories(environment, binary);
            QStringList expectedVendor;
            for (const QJsonValue &entry : expected.value(QStringLiteral("dll_dirs")).toArray()) {
                expectedVendor.append(entry.toString());
            }
            QCOMPARE(vendor, expectedVendor);

            QProcessEnvironment base;
            base.insert(QStringLiteral("PATH"), input.value(QStringLiteral("env_base_path")).toString());
            QVector<int> gpuIds;
            for (const QJsonValue &id : input.value(QStringLiteral("gpu_ids")).toArray()) {
                gpuIds.append(id.toInt());
            }
            const QProcessEnvironment built = ServerEnv::build(base, vendor, gpuIds);
            const QJsonObject delta = expected.value(QStringLiteral("server_env_delta")).toObject();
            for (auto key = delta.begin(); key != delta.end(); ++key) {
                QCOMPARE(built.value(key.key()), key.value().toString());
            }
            QCOMPARE(built.keys().size(), base.keys().size() + 2);
        }
    }

    void probeHidesEveryGpu() {
        QProcessEnvironment base;
        base.insert(QStringLiteral("PATH"), QStringLiteral("C:\\x"));
        const QProcessEnvironment probe = ServerEnv::forProbe(base, {QStringLiteral("C:\\v")});
        QCOMPARE(probe.value(QStringLiteral("CUDA_VISIBLE_DEVICES")), QStringLiteral("-1"));
        QCOMPARE(probe.value(QStringLiteral("PATH")), QStringLiteral("C:\\v;C:\\x"));
        QCOMPARE(probe.value(QStringLiteral("CUDA_DEVICE_ORDER")), QStringLiteral("PCI_BUS_ID"));
    }
};

QTEST_APPLESS_MAIN(TestServerEnv)
#include "test_server_env.moc"

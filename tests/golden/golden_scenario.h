#pragma once

#include "fake_server_environment.h"
#include "golden_model_source.h"
#include "golden_models.h"

#include "common/model_names.h"
#include "launch/launch_assembler.h"
#include "launch/line_splitter.h"
#include "launch/run_planner.h"
#include "launch/run_settings_parser.h"
#include "launch/server_env.h"
#include "launch/server_exit.h"
#include "moe/local_usage_source.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

#include <windows.h>

// One scenario through the C++ Run path (README section 5), run in the current directory, which
// the caller makes an empty scratch folder: the files_before are materialized there and the real
// hot-file and usage-file I/O happens there, exactly as Python's did in _work/run.
class GoldenScenario {
public:
    GoldenScenario(const GoldenModels &models, const QString &usageFixtures) : m_models(models), m_usage(usageFixtures) {}

    QJsonObject run(const QJsonObject &fixture) const {
        // the exporter resets the usage folder to empty before every scenario
        QDir().mkpath(fixture.value(QStringLiteral("usage_dir")).toString());
        materialize(fixture.value(QStringLiteral("files_before")).toArray());
        QJsonObject actual{{QStringLiteral("log"), QJsonArray()}};
        const auto settings = RunSettingsParser::parse(fixture.value(QStringLiteral("settings")).toObject().toVariantMap());
        if (!settings.ok()) {
            actual.insert(QStringLiteral("error"), settings.error());
            actual.insert(QStringLiteral("error_stage"), QStringLiteral("read_settings"));
            actual.insert(QStringLiteral("files_after"), filesAfter(fixture));
            return actual;
        }
        QStringList log;
        const QString binary = fixture.value(QStringLiteral("resolved_binary")).toString();
        const QProcessEnvironment base = environmentBase(fixture);
        QSet<QString> flags;
        for (const QJsonValue &flag : fixture.value(QStringLiteral("flags")).toArray()) {
            flags.insert(flag.toString());
        }
        const QProcessEnvironment serverEnvironment = ServerEnv::build(
            base, ServerEnv::vendorDllDirectories(vendorTree(fixture), binary), settings.value().gpuIds);

        const GoldenModelSource models(m_models, fixture.value(QStringLiteral("model_fs")).toObject());
        const LocalUsageSource usage;
        RunEnvironment environment;
        environment.models = &models;
        environment.usage = &usage;
        environment.hardware = hardware(fixture);
        environment.flags = flags;
        environment.modelsDirectory = fixture.value(QStringLiteral("models_dir")).toString();
        environment.usageDirectory = fixture.value(QStringLiteral("usage_dir")).toString();
        environment.hotDirectory = fixture.value(QStringLiteral("hot_dir")).toString();

        RunPlan run = RunPlanner::plan(settings.value(), environment);
        log += run.logLines;
        QString error = run.error;
        const auto hotWritten = LaunchAssembler::writeHotFile(run, environment.hotDirectory, settings.value().modelPath);
        if (run.ok() && !hotWritten.ok()) {
            error = hotWritten.error();
            log.append(RunPlanner::criticalLine(error));
            run.plan.reset();
        }
        if (run.ok()) {
            const auto launch = LaunchAssembler::assemble(settings.value(), run, binary, flags, serverEnvironment,
                                                          environment.usageDirectory);
            if (launch.ok()) {
                log += launch.value().logLines;
                actual.insert(QStringLiteral("argv"), QJsonArray::fromStringList(launch.value().argv));
                actual.insert(QStringLiteral("env_delta"), delta(base, launch.value().environment));
                log += processLines(fixture.value(QStringLiteral("process")).toObject());
            } else {
                error = launch.error();
                log.append(RunPlanner::criticalLine(error));
            }
        }
        if (run.plan) {
            insertPlan(actual, *run.plan);
        }
        const QString hotPath = run.hotPathToPrune;
        actual.insert(QStringLiteral("hot_path"), hotPath.isEmpty() ? QJsonValue() : ModelNames::baseName(hotPath));
        actual.insert(QStringLiteral("hot_file_text"), hotPath.isEmpty() ? QJsonValue() : fileText(hotPath));
        actual.insert(QStringLiteral("log"), QJsonArray::fromStringList(log));
        actual.insert(QStringLiteral("error"), error.isEmpty() ? QJsonValue() : error);
        actual.insert(QStringLiteral("error_stage"), error.isEmpty() ? QJsonValue() : QStringLiteral("run_process"));
        actual.insert(QStringLiteral("files_after"), filesAfter(fixture));
        return actual;
    }

private:
    void materialize(const QJsonArray &files) const {
        for (const QJsonValue &value : files) {
            const QJsonObject file = value.toObject();
            const QString path = file.value(QStringLiteral("path")).toString();
            QDir().mkpath(QFileInfo(path).path());
            QByteArray bytes = file.value(QStringLiteral("text")).toString().toUtf8();
            const QString profile = file.value(QStringLiteral("usage_profile")).toString();
            if (!profile.isEmpty()) {
                QFile source(m_usage + u'/' + profile + QStringLiteral(".usage.json"));
                bytes = source.open(QIODevice::ReadOnly) ? source.readAll() : QByteArray();
            }
            QFile target(path);
            if (target.open(QIODevice::WriteOnly)) {
                target.write(bytes);
                target.close();
            }
            if (file.contains(QStringLiteral("mtime_ns"))) {
                setModifiedTime(path, file.value(QStringLiteral("mtime_ns")).toInteger());
            }
        }
    }

    // NTFS keeps 100 ns ticks; Qt's setFileTime only takes milliseconds, and the hot-cache key
    // hashes the full st_mtime_ns.
    static void setModifiedTime(const QString &path, qint64 nanoseconds) {
        constexpr qint64 EPOCH_TICKS = 116444736000000000;
        const qint64 ticks = nanoseconds / 100 + EPOCH_TICKS;
        FILETIME stamp{DWORD(ticks & 0xFFFFFFFF), DWORD(ticks >> 32)};
        const std::wstring native = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath()).toStdWString();
        HANDLE handle = CreateFileW(native.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle != INVALID_HANDLE_VALUE) {
            SetFileTime(handle, nullptr, nullptr, &stamp);
            CloseHandle(handle);
        }
    }

    static QProcessEnvironment environmentBase(const QJsonObject &fixture) {
        QProcessEnvironment base;
        const QJsonObject values = fixture.value(QStringLiteral("env_base")).toObject();
        for (auto it = values.begin(); it != values.end(); ++it) {
            base.insert(it.key(), it.value().toString());
        }
        return base;
    }

    static FakeServerEnvironment vendorTree(const QJsonObject &fixture) {
        FakeServerEnvironment tree;
        const QJsonObject directories = fixture.value(QStringLiteral("virtual_dirs")).toObject();
        for (auto it = directories.begin(); it != directories.end(); ++it) {
            for (const QJsonValue &entry : it.value().toArray()) {
                tree.addFile(it.key() + u'\\' + entry.toString());
            }
        }
        return tree;
    }

    static HardwareSnapshot hardware(const QJsonObject &fixture) {
        HardwareSnapshot snapshot;
        for (const QJsonValue &value : fixture.value(QStringLiteral("gpu_snapshot")).toArray()) {
            const QJsonObject gpu = value.toObject();
            snapshot.gpus.append({gpu.value(QStringLiteral("index")).toInt(), gpu.value(QStringLiteral("name")).toString(),
                                  gpu.value(QStringLiteral("free_mib")).toInteger(),
                                  gpu.value(QStringLiteral("total_mib")).toInteger()});
        }
        const QJsonValue ram = fixture.value(QStringLiteral("free_ram_bytes"));
        if (!ram.isNull()) {
            snapshot.freeRamBytes = ram.toInteger();
        }
        return snapshot;
    }

    static QJsonObject delta(const QProcessEnvironment &base, const QProcessEnvironment &final) {
        QJsonObject changed;
        for (const QString &key : final.keys()) {
            if (!base.contains(key) || base.value(key) != final.value(key)) {
                changed.insert(key, final.value(key));
            }
        }
        return changed;
    }

    // Python iterated the pipe in text mode: universal newlines, each line keeping its "\n".
    static QStringList processLines(const QJsonObject &process) {
        QByteArray output;
        for (const QJsonValue &line : process.value(QStringLiteral("stdout")).toArray()) {
            output += line.toString().toUtf8();
        }
        LineSplitter splitter;
        QStringList lines;
        for (const QString &line : splitter.feed(output)) {
            lines.append(line + u'\n');
        }
        lines += splitter.finish();
        return lines + ServerExit::launcherLines(quint32(process.value(QStringLiteral("exit_code")).toInteger()));
    }

    static void insertPlan(QJsonObject &actual, const Plan &plan) {
        actual.insert(QStringLiteral("args"), QJsonArray::fromStringList(plan.args));
        actual.insert(QStringLiteral("report"), QJsonArray::fromStringList(plan.report));
        actual.insert(QStringLiteral("warnings"), QJsonArray::fromStringList(plan.warnings));
        actual.insert(QStringLiteral("use_mmap"), plan.useMmap);
        actual.insert(QStringLiteral("ubatch"), plan.microBatch);
        actual.insert(QStringLiteral("arch"), plan.arch);
    }

    static QJsonValue fileText(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QJsonValue(QString::fromUtf8(file.readAll())) : QJsonValue();
    }

    static QJsonObject filesAfter(const QJsonObject &fixture) {
        return {{QStringLiteral("usage_dir"), listing(fixture.value(QStringLiteral("usage_dir")).toString())},
                {QStringLiteral("hot_dir"), listing(fixture.value(QStringLiteral("hot_dir")).toString())}};
    }

    static QJsonValue listing(const QString &directory) {
        if (!QFileInfo(directory).isDir()) {
            return QJsonValue();
        }
        QStringList names = QDir(directory).entryList(QDir::Files | QDir::Hidden | QDir::System);
        names.sort(Qt::CaseSensitive);
        return QJsonArray::fromStringList(names);
    }

    const GoldenModels &m_models;
    QString m_usage;
};

#include "gguf/layout_reader.h"
#include "gguf/local_model_source.h"
#include "plan/placement_planner.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

// Live parity tool (02 section 8): reads scenarios as JSON lines
// {"model", "gpu_ids", "ctx", "margin", "allow_ssd", "ubatch" ("auto"|int), "reserve_last",
//  "gpus": [{"index","name","free_mib","total_mib"}], "free_ram": int|null}
// and prints one plan JSON line per scenario, in the field names of strata_planner.Plan.
namespace {

HardwareSnapshot hardwareFrom(const QJsonObject &scenario) {
    HardwareSnapshot hardware;
    for (const QJsonValue &value : scenario.value(QStringLiteral("gpus")).toArray()) {
        const QJsonObject gpu = value.toObject();
        hardware.gpus.append({gpu.value(QStringLiteral("index")).toInt(), gpu.value(QStringLiteral("name")).toString(),
                              qint64(gpu.value(QStringLiteral("free_mib")).toDouble()),
                              qint64(gpu.value(QStringLiteral("total_mib")).toDouble())});
    }
    const QJsonValue ram = scenario.value(QStringLiteral("free_ram"));
    if (!ram.isNull() && !ram.isUndefined()) {
        hardware.freeRamBytes = qint64(ram.toDouble());
    }
    return hardware;
}

QJsonObject planJson(const QJsonObject &scenario, const ModelLayout &layout) {
    PlanRequest request;
    request.layout = layout;
    for (const QJsonValue &id : scenario.value(QStringLiteral("gpu_ids")).toArray()) {
        request.gpuIds.append(id.toInt());
    }
    request.contextSize = qint64(scenario.value(QStringLiteral("ctx")).toDouble());
    request.marginMib = qint64(scenario.value(QStringLiteral("margin")).toDouble());
    request.allowSsd = scenario.value(QStringLiteral("allow_ssd")).toBool(true);
    const QJsonValue microBatch = scenario.value(QStringLiteral("ubatch"));
    if (microBatch.isDouble()) {
        request.microBatch = microBatch.toInt();
    }
    request.reserveLastBytes = qint64(scenario.value(QStringLiteral("reserve_last")).toDouble());

    const auto plan = PlacementPlanner::plan(request, hardwareFrom(scenario));
    if (!plan.ok()) {
        return {{QStringLiteral("error"), plan.error()}};
    }
    return {{QStringLiteral("args"), QJsonArray::fromStringList(plan.value().args)},
            {QStringLiteral("report"), QJsonArray::fromStringList(plan.value().report)},
            {QStringLiteral("warnings"), QJsonArray::fromStringList(plan.value().warnings)},
            {QStringLiteral("use_mmap"), plan.value().useMmap},
            {QStringLiteral("ubatch"), plan.value().microBatch},
            {QStringLiteral("arch"), plan.value().arch}};
}

} // namespace

int main(int argc, char *argv[]) {
    QCoreApplication application(argc, argv);
    QFile input;
    if (!input.open(stdin, QIODevice::ReadOnly)) {
        return 1;
    }
    QTextStream out(stdout);
    const LocalModelSource source;
    QMap<QString, ModelLayout> layouts;
    while (!input.atEnd()) {
        const QJsonObject scenario = QJsonDocument::fromJson(input.readLine()).object();
        const QString model = scenario.value(QStringLiteral("model")).toString();
        if (!layouts.contains(model)) {
            const auto layout = LayoutReader::read(source, model);
            if (!layout.ok()) {
                out << QJsonDocument(QJsonObject{{QStringLiteral("error"), layout.error()}}).toJson(QJsonDocument::Compact) << '\n';
                continue;
            }
            layouts.insert(model, layout.value());
        }
        out << QJsonDocument(planJson(scenario, layouts.value(model))).toJson(QJsonDocument::Compact) << '\n';
        out.flush();
    }
    return 0;
}

#include "moe/hot_builder.h"
#include "moe/hot_cache.h"
#include "moe/hot_file.h"
#include "moe/local_usage_source.h"
#include "moe/usage_io.h"

#include "common/model_names.h"
#include "gguf/model_layout.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

#include <cstring>

// Parity tool for core/moe (02 section 8): one JSON scenario per stdin line, one JSON result per
// stdout line, compared against moe_hot.py / hot_cache_path by an out-of-repo Python harness.
// Doubles are printed as the hex of their IEEE bits so the comparison is exact.
namespace {

QString bits(double value) {
    quint64 raw = 0;
    std::memcpy(&raw, &value, sizeof raw);
    return QString::number(raw, 16).rightJustified(16, u'0');
}

QVector<qint64> int64s(const QJsonValue &value) {
    QVector<qint64> out;
    for (const QJsonValue &item : value.toArray()) {
        out.append(item.toInteger());
    }
    return out;
}

QVector<int> ints(const QJsonValue &value) {
    QVector<int> out;
    for (const QJsonValue &item : value.toArray()) {
        out.append(item.toInt());
    }
    return out;
}

QStringList strings(const QJsonValue &value) {
    QStringList out;
    for (const QJsonValue &item : value.toArray()) {
        out.append(item.toString());
    }
    return out;
}

ModelLayout layoutFrom(const QJsonObject &json) {
    ModelLayout layout;
    layout.arch = json.value(QStringLiteral("arch")).toString();
    layout.layerCount = json.value(QStringLiteral("n_layer")).toInt();
    layout.skippedLayerCount = json.value(QStringLiteral("n_skipped")).toInt();
    layout.expertCount = json.value(QStringLiteral("n_expert")).toInt();
    layout.expertUsedCount = json.value(QStringLiteral("n_expert_used")).toInt();
    layout.expertBytes = int64s(json.value(QStringLiteral("experts")));
    layout.denseBytes = int64s(json.value(QStringLiteral("dense")));
    return layout;
}

QJsonObject hotJson(const HotMap &hot) {
    QJsonObject out;
    for (auto layer = hot.cbegin(); layer != hot.cend(); ++layer) {
        QJsonArray experts;
        for (const int expert : layer.value()) {
            experts.append(expert);
        }
        out.insert(QString::number(layer.key()), experts);
    }
    return out;
}

QJsonObject buildScenario(const QJsonObject &scenario) {
    const ModelLayout layout = layoutFrom(scenario.value(QStringLiteral("layout")).toObject());
    const auto counts = UsageIo::readCounts(LocalUsageSource(), strings(scenario.value(QStringLiteral("files"))));
    if (!counts.ok()) {
        return {{QStringLiteral("error"), counts.error()}};
    }
    const QVector<int> starts = ints(scenario.value(QStringLiteral("starts")));
    const QVector<int> sizes = ints(scenario.value(QStringLiteral("sizes")));
    const QVector<qint64> spare = int64s(scenario.value(QStringLiteral("spare")));
    const QVector<int> cpuLayers = ints(scenario.value(QStringLiteral("cpu_layers")));
    const double bandwidth = scenario.value(QStringLiteral("bw")).toDouble();
    const double overhead = scenario.value(QStringLiteral("ovh")).toDouble();

    const HotMap greedy = HotBuilder::buildGreedy(layout, counts.value(), starts, sizes, spare);
    const auto mixed = HotBuilder::buildCost(layout, counts.value(), starts, sizes, spare, bandwidth, overhead, true);
    const auto whole = HotBuilder::buildCost(layout, counts.value(), starts, sizes, spare, bandwidth, overhead, false);
    const QVector<int> mixedOrder = HotBuilder::costInsertionOrder(mixed.first, starts, sizes);
    const QByteArray mixedText = HotFile::write(layout.expertCount, mixed.first);
    const auto readBack = HotFile::read(mixedText, layout.expertCount);
    QJsonObject out{
        {QStringLiteral("greedy"), hotJson(greedy)},
        {QStringLiteral("mixed"), hotJson(mixed.first)},
        {QStringLiteral("mixed_ms"), bits(mixed.second)},
        {QStringLiteral("whole"), hotJson(whole.first)},
        {QStringLiteral("whole_ms"), bits(whole.second)},
        {QStringLiteral("greedy_hit"), bits(HotBuilder::hitRate(layout, counts.value(), greedy))},
        {QStringLiteral("mixed_hit"), bits(HotBuilder::hitRate(layout, counts.value(), mixed.first, mixedOrder))},
        {QStringLiteral("greedy_predict"),
         bits(HotBuilder::predictMs(layout, counts.value(), greedy, {}, bandwidth, overhead))},
        {QStringLiteral("mixed_predict"),
         bits(HotBuilder::predictMs(layout, counts.value(), mixed.first, {}, bandwidth, overhead, mixedOrder))},
        {QStringLiteral("base_predict"),
         bits(HotBuilder::predictMs(layout, counts.value(), {}, cpuLayers, bandwidth, overhead))},
        {QStringLiteral("greedy_text"), QString::fromLatin1(HotFile::write(layout.expertCount, greedy))},
        {QStringLiteral("mixed_text"), QString::fromLatin1(mixedText)},
        {QStringLiteral("read_back_ok"), readBack.ok() && readBack.value() == mixed.first},
    };
    return out;
}

QJsonObject tokensScenario(const QJsonObject &scenario) {
    const auto tokens = UsageIo::usageTokens(LocalUsageSource(), strings(scenario.value(QStringLiteral("files"))));
    if (!tokens.ok()) {
        return {{QStringLiteral("error"), tokens.error()}};
    }
    return {{QStringLiteral("tokens"), QJsonArray{tokens.value().first, tokens.value().second}}};
}

FileStamp stampFrom(const QJsonValue &value) {
    const QJsonArray array = value.toArray();
    return {array.at(0).toString(), array.at(1).toInteger(), array.at(2).toInteger()};
}

QJsonObject cacheScenario(const QJsonObject &scenario) {
    HotCacheKey key;
    key.model = stampFrom(scenario.value(QStringLiteral("model")));
    for (const QJsonValue &file : scenario.value(QStringLiteral("files")).toArray()) {
        key.usageFiles.append(stampFrom(file));
    }
    key.gpuIds = ints(scenario.value(QStringLiteral("gpu_ids")));
    key.contextSize = scenario.value(QStringLiteral("ctx")).toInteger();
    key.marginMib = scenario.value(QStringLiteral("margin")).toInteger();
    const QJsonValue microBatch = scenario.value(QStringLiteral("ubatch"));
    if (microBatch.isDouble()) {
        key.microBatch = microBatch.toInt();
    }
    key.reserveBytes = scenario.value(QStringLiteral("reserve")).toInteger();
    const QString hotDirectory = scenario.value(QStringLiteral("hot_dir")).toString();
    const QString path = HotCache::filePath(hotDirectory, key);
    return {{QStringLiteral("key_json"), HotCache::keyJson(key)},
            {QStringLiteral("path"), path},
            {QStringLiteral("model_key"),
             ModelNames::modelKey(key.model.path, key.model.sizeBytes, key.model.modifiedTimeNs)},
            {QStringLiteral("stem"), ModelNames::modelStem(key.model.path)},
            {QStringLiteral("stale"),
             QJsonArray::fromStringList(HotCache::staleFiles(
                 hotDirectory, strings(scenario.value(QStringLiteral("existing"))), key.model.path, path))}};
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
    while (!input.atEnd()) {
        const QByteArray line = input.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const QJsonObject scenario = QJsonDocument::fromJson(line).object();
        const QString op = scenario.value(QStringLiteral("op")).toString();
        QJsonObject result;
        if (op == QStringLiteral("build")) {
            result = buildScenario(scenario);
        } else if (op == QStringLiteral("tokens")) {
            result = tokensScenario(scenario);
        } else {
            result = cacheScenario(scenario);
        }
        out << QJsonDocument(result).toJson(QJsonDocument::Compact) << '\n';
        out.flush();
    }
    return 0;
}

#include "gguf/draft_finder.h"
#include "gguf/layout_reader.h"
#include "gguf/local_model_source.h"
#include "gguf/mmproj_finder.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

// Prints read_layout of a real model as the JSON dataclasses.asdict(read_layout(path)) gives in
// Python, for side-by-side parity checks on files too big to commit (02 section 8).
namespace {

QJsonArray toJson(const QVector<qint64> &values) {
    QJsonArray array;
    for (const qint64 value : values) {
        array.append(value);
    }
    return array;
}

QJsonObject toJson(const ModelLayout &layout) {
    return {
        {QStringLiteral("arch"), layout.arch},
        {QStringLiteral("n_layer"), layout.layerCount},
        {QStringLiteral("n_skipped"), layout.skippedLayerCount},
        {QStringLiteral("n_expert"), layout.expertCount},
        {QStringLiteral("n_expert_used"), layout.expertUsedCount},
        {QStringLiteral("total_bytes"), layout.totalBytes},
        {QStringLiteral("dense"), toJson(layout.denseBytes)},
        {QStringLiteral("experts"), toJson(layout.expertBytes)},
        {QStringLiteral("output_bytes"), layout.outputBytes},
        {QStringLiteral("cpu_global_bytes"), layout.cpuGlobalBytes},
        {QStringLiteral("kv_per_token"), toJson(layout.kvBytesPerToken)},
        {QStringLiteral("n_embd"), layout.embeddingLength},
        {QStringLiteral("n_vocab"), layout.vocabSize},
        {QStringLiteral("eog_overrides"), QJsonArray::fromStringList(layout.stopTokenOverrides)},
        {QStringLiteral("has_indexer"), layout.hasIndexer},
    };
}

} // namespace

int main(int argc, char *argv[]) {
    QCoreApplication application(argc, argv);
    QTextStream out(stdout);
    const LocalModelSource source;
    for (const QString &path : QCoreApplication::arguments().mid(1)) {
        QElapsedTimer timer;
        timer.start();
        const auto layout = LayoutReader::read(source, path);
        const qint64 elapsed = timer.elapsed();
        if (!layout.ok()) {
            out << "ERROR " << layout.error() << '\n';
            continue;
        }
        out << QJsonDocument(toJson(layout.value())).toJson(QJsonDocument::Compact) << '\n';
        out << "ms " << elapsed << '\n';
        out << "draft " << DraftFinder::find(source, path, {}).value_or(QStringLiteral("none")) << '\n';
        out << "mmproj " << MmprojFinder::find(source, path, {}).value_or(QStringLiteral("none")) << '\n';
    }
    return 0;
}

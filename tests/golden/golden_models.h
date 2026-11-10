#pragma once

#include "gguf/shard_cache.h"

#include <QByteArray>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <optional>

// The fixture GGUFs (tests/golden/fixtures/models, README section 2), keyed by the path Python
// saw. Headers are decompressed on first use and kept, so 182 scenarios read each model once.
class GoldenModels {
public:
    struct Model {
        QString id;
        qint64 size = 0;
        qint64 modifiedTimeNs = 0;
    };

    explicit GoldenModels(const QString &modelsDirectory) : m_directory(modelsDirectory) {
        const QJsonArray models = readJson(m_directory + QStringLiteral("/index.json")).value(QStringLiteral("models")).toArray();
        for (const QJsonValue &value : models) {
            const QString id = value.toObject().value(QStringLiteral("id")).toString();
            const QJsonObject sidecar = readJson(m_directory + u'/' + id + QStringLiteral(".json"));
            m_models.insert(sidecar.value(QStringLiteral("original_path")).toString(),
                            {id, sidecar.value(QStringLiteral("size")).toInteger(),
                             sidecar.value(QStringLiteral("mtime_ns")).toInteger()});
        }
    }

    std::optional<Model> find(const QString &path) const {
        const auto found = m_models.constFind(path);
        return found == m_models.cend() ? std::nullopt : std::optional<Model>(*found);
    }

    bool hasModelUnder(const QString &directory) const {
        const QString prefix = directory.endsWith(u'\\') ? directory : directory + u'\\';
        for (auto it = m_models.cbegin(); it != m_models.cend(); ++it) {
            if (it.key().startsWith(prefix, Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    }

    QByteArray bytes(const QString &id) const {
        auto cached = m_bytes.constFind(id);
        if (cached == m_bytes.cend()) {
            QFile file(m_directory + u'/' + id + QStringLiteral(".gguf.z"));
            const QByteArray compressed = file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
            cached = m_bytes.insert(id, qUncompress(compressed));
        }
        return *cached;
    }

    // Large enough for every fixture GGUF: each header is parsed once for the whole run.
    ShardCache *shardCache() const { return &m_shardCache; }

    static QJsonObject readJson(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
    }

private:
    QString m_directory;
    QHash<QString, Model> m_models;
    mutable QHash<QString, QByteArray> m_bytes;
    mutable ShardCache m_shardCache{64};
};

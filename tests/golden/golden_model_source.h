#pragma once

#include "golden_models.h"

#include "common/path_key.h"
#include "gguf/model_source.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>

#include <algorithm>

// ModelSource for one scenario: the facts strata_planner recorded (model_fs, README 5.1) first,
// then the fixture models themselves. Paths are compared as the strings Python used.
class GoldenModelSource : public ModelSource {
public:
    GoldenModelSource(const GoldenModels &models, const QJsonObject &modelFs) : m_models(models) {
        const auto boolMap = [&modelFs](const char *key) {
            QMap<QString, bool> out;
            const QJsonObject object = modelFs.value(QLatin1String(key)).toObject();
            for (auto it = object.begin(); it != object.end(); ++it) {
                out.insert(it.key(), it.value().toBool());
            }
            return out;
        };
        m_exists = boolMap("exists");
        m_isDirectory = boolMap("isdir");
        const QJsonObject sizes = modelFs.value(QStringLiteral("getsize")).toObject();
        for (auto it = sizes.begin(); it != sizes.end(); ++it) {
            m_sizes.insert(it.key(), it.value().toInteger());
        }
        for (const QJsonValue &value : modelFs.value(QStringLiteral("globs")).toArray()) {
            const QJsonObject glob = value.toObject();
            QStringList results;
            for (const QJsonValue &result : glob.value(QStringLiteral("results")).toArray()) {
                results.append(result.toArray().at(0).toString());
                m_sizes.insert(result.toArray().at(0).toString(), result.toArray().at(1).toInteger());
            }
            m_globs.insert(glob.value(QStringLiteral("pattern")).toString() + u'|'
                               + (glob.value(QStringLiteral("recursive")).toBool() ? u'r' : u'f'),
                           results);
        }
    }

    bool exists(const QString &path) const override {
        return m_exists.contains(path) ? m_exists.value(path) : m_models.find(path).has_value();
    }
    bool isDirectory(const QString &path) const override {
        return m_isDirectory.contains(path) ? m_isDirectory.value(path) : m_models.hasModelUnder(path);
    }
    std::optional<qint64> fileSize(const QString &path) const override {
        if (m_sizes.contains(path)) {
            return m_sizes.value(path);
        }
        const auto model = m_models.find(path);
        return model ? std::optional<qint64>(model->size) : std::nullopt;
    }
    std::optional<qint64> modifiedTimeNs(const QString &path) const override {
        const auto model = m_models.find(path);
        return model ? std::optional<qint64>(model->modifiedTimeNs) : std::nullopt;
    }
    std::optional<QByteArray> readPrefix(const QString &path, qint64 maxBytes) const override {
        const auto model = m_models.find(path);
        if (!model) {
            return std::nullopt;
        }
        const QByteArray bytes = m_models.bytes(model->id);
        return bytes.left(qsizetype(std::min<qint64>(maxBytes, bytes.size())));
    }
    ShardCache *shardCache() const override { return m_models.shardCache(); }

    // Python globbed os.path.join(dir, "**", pattern) (recursive) or os.path.join(dir, pattern).
    QStringList listFiles(const QString &directory, const QString &pattern, bool isRecursive) const override {
        const QString joined = isRecursive ? path_key::join(path_key::join(directory, QStringLiteral("**")), pattern)
                                           : path_key::join(directory, pattern);
        return m_globs.value(joined + u'|' + (isRecursive ? u'r' : u'f'));
    }

private:
    const GoldenModels &m_models;
    QMap<QString, bool> m_exists;
    QMap<QString, bool> m_isDirectory;
    QMap<QString, qint64> m_sizes;
    QMap<QString, QStringList> m_globs;
};

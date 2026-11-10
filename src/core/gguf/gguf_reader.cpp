#include "gguf/gguf_reader.h"

#include "gguf/model_source.h"
#include "gguf/shard_cache.h"

#include <QFileInfo>
#include <QVariantList>

#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

namespace {

constexpr quint32 TYPE_BOOL = 7;
constexpr quint32 TYPE_STRING = 8;
constexpr quint32 TYPE_ARRAY = 9;
constexpr quint64 MAX_DECODED_STRING_ARRAY = 64;
constexpr qint64 FIRST_READ_BYTES = 4 * 1024 * 1024;
constexpr qint64 GROWTH_FACTOR = 4;

// Little-endian cursor over the header bytes. Running past the end sets isTruncated instead of
// failing, so the caller can fetch a longer prefix and retry.
class Cursor {
public:
    explicit Cursor(const QByteArray &bytes) : m_bytes(bytes) {}

    bool isTruncated() const { return m_isTruncated; }
    qint64 position() const { return m_position; }

    template <typename T>
    T read() {
        T value{};
        if (!has(sizeof(T))) {
            return value;
        }
        std::memcpy(&value, m_bytes.constData() + m_position, sizeof(T));
        m_position += sizeof(T);
        return value;
    }

    QString readString() {
        const auto length = read<quint64>();
        if (!has(length)) {
            return {};
        }
        const QString text = QString::fromUtf8(m_bytes.constData() + m_position, qsizetype(length));
        m_position += qint64(length);
        return text;
    }

    void skip(quint64 count) {
        if (has(count)) {
            m_position += qint64(count);
        }
    }

private:
    bool has(quint64 count) {
        if (m_isTruncated || quint64(m_bytes.size() - m_position) < count) {
            m_isTruncated = true;
            return false;
        }
        return true;
    }

    const QByteArray &m_bytes;
    qint64 m_position = 0;
    bool m_isTruncated = false;
};

bool isScalarType(quint32 type) {
    return type <= TYPE_BOOL || (type >= 10 && type <= 12);
}

QVariant readScalar(Cursor &cursor, quint32 type) {
    switch (type) {
    case 0: return qulonglong(cursor.read<quint8>());
    case 1: return qlonglong(cursor.read<qint8>());
    case 2: return qulonglong(cursor.read<quint16>());
    case 3: return qlonglong(cursor.read<qint16>());
    case 4: return qulonglong(cursor.read<quint32>());
    case 5: return qlonglong(cursor.read<qint32>());
    case 6: return double(cursor.read<float>());
    case TYPE_BOOL: return cursor.read<quint8>() != 0;
    case 10: return qulonglong(cursor.read<quint64>());
    case 11: return qlonglong(cursor.read<qint64>());
    default: return cursor.read<double>();
    }
}

Result<QVariant> readValue(Cursor &cursor, quint32 type, bool keepStrings);

Result<QVariant> readArray(Cursor &cursor, bool keepStrings) {
    const auto itemType = cursor.read<quint32>();
    const auto count = cursor.read<quint64>();
    if (itemType == TYPE_STRING && count > MAX_DECODED_STRING_ARRAY && !keepStrings) {
        for (quint64 i = 0; i < count && !cursor.isTruncated(); ++i) {
            cursor.skip(cursor.read<quint64>());
        }
        return Result<QVariant>::success(QVariant());
    }
    QVariantList items;
    for (quint64 i = 0; i < count && !cursor.isTruncated(); ++i) {
        // Nested values never keep large string arrays, matching the Python default argument.
        auto item = readValue(cursor, itemType, false);
        if (!item.ok()) {
            return item;
        }
        items.append(item.value());
    }
    return Result<QVariant>::success(items);
}

Result<QVariant> readValue(Cursor &cursor, quint32 type, bool keepStrings) {
    if (isScalarType(type)) {
        return Result<QVariant>::success(readScalar(cursor, type));
    }
    if (type == TYPE_STRING) {
        return Result<QVariant>::success(cursor.readString());
    }
    if (type == TYPE_ARRAY) {
        return readArray(cursor, keepStrings);
    }
    return Result<QVariant>::failure(QStringLiteral("unknown GGUF value type %1").arg(type));
}

QMap<QString, qint64> sizesFromOffsets(std::vector<std::pair<quint64, QString>> infos, qint64 dataEnd) {
    // Python sorts (offset, name) tuples, so equal offsets order by name.
    std::sort(infos.begin(), infos.end());
    QMap<QString, qint64> sizes;
    for (size_t i = 0; i < infos.size(); ++i) {
        const qint64 next = i + 1 < infos.size() ? qint64(infos[i + 1].first) : dataEnd;
        sizes.insert(infos[i].second, next - qint64(infos[i].first));
    }
    return sizes;
}

} // namespace

Result<GgufShard> GgufReader::parse(const QByteArray &header, qint64 fileSize, const QString &fileName,
                                    bool *isTruncated) {
    *isTruncated = false;
    Cursor cursor(header);
    if (header.left(4) != QByteArrayLiteral("GGUF")) {
        return Result<GgufShard>::failure(QStringLiteral("%1 is not a GGUF file").arg(fileName));
    }
    cursor.skip(4);
    const auto version = cursor.read<quint32>();
    if (version < 2) {
        return Result<GgufShard>::failure(QStringLiteral("GGUF version %1 is too old").arg(version));
    }
    const auto tensorCount = cursor.read<quint64>();
    const auto keyCount = cursor.read<quint64>();

    GgufShard shard;
    for (quint64 i = 0; i < keyCount && !cursor.isTruncated(); ++i) {
        const QString key = cursor.readString();
        const auto value = readValue(cursor, cursor.read<quint32>(), key == QLatin1String("tokenizer.ggml.tokens"));
        if (!value.ok()) {
            return Result<GgufShard>::failure(value.error());
        }
        shard.metadata.insert(key, value.value());
    }

    std::vector<std::pair<quint64, QString>> infos;
    for (quint64 i = 0; i < tensorCount && !cursor.isTruncated(); ++i) {
        QString name = cursor.readString();
        const auto dimensionCount = cursor.read<quint32>();
        cursor.skip(8 * quint64(dimensionCount) + 4);
        infos.emplace_back(cursor.read<quint64>(), std::move(name));
    }
    if (cursor.isTruncated()) {
        *isTruncated = true;
        return Result<GgufShard>::failure(QStringLiteral("%1 ends inside its GGUF header").arg(fileName));
    }

    const qint64 alignment = shard.metadata.value(QStringLiteral("general.alignment"), 32).toLongLong();
    const qint64 dataStart = (cursor.position() + alignment - 1) / alignment * alignment;
    shard.tensorSizes = sizesFromOffsets(std::move(infos), fileSize - dataStart);
    return Result<GgufShard>::success(std::move(shard));
}

Result<GgufShard> GgufReader::read(const ModelSource &source, const QString &path) {
    const auto fileSize = source.fileSize(path);
    if (!fileSize) {
        return Result<GgufShard>::failure(QStringLiteral("Cannot read %1.").arg(path));
    }
    ShardCache *cache = source.shardCache();
    const QString key = ShardCache::keyFor(path, *fileSize, source.modifiedTimeNs(path).value_or(-1));
    if (cache) {
        if (auto cached = cache->find(key)) {
            return Result<GgufShard>::success(std::move(*cached));
        }
    }
    auto shard = readUncached(source, path, *fileSize);
    if (cache && shard.ok()) {
        cache->insert(key, shard.value());
    }
    return shard;
}

Result<GgufShard> GgufReader::readUncached(const ModelSource &source, const QString &path, qint64 knownSize) {
    const QString fileName = QFileInfo(path).fileName();
    const std::optional<qint64> fileSize = knownSize;
    // Headers run from a few KB to ~11 MB (tokenizer arrays); grow the prefix only when needed.
    for (qint64 want = FIRST_READ_BYTES;; want *= GROWTH_FACTOR) {
        const auto prefix = source.readPrefix(path, std::min(want, *fileSize));
        if (!prefix) {
            return Result<GgufShard>::failure(QStringLiteral("Cannot read %1.").arg(path));
        }
        bool isTruncated = false;
        auto shard = parse(*prefix, *fileSize, fileName, &isTruncated);
        if (!isTruncated || want >= *fileSize) {
            return shard;
        }
    }
}

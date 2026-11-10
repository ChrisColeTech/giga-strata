#pragma once

#include <QByteArray>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

#include <cstring>

// Writes small GGUF headers for unit tests: metadata, a tensor table, alignment padding and
// optional tensor data bytes.
class GgufBuilder {
public:
    explicit GgufBuilder(quint32 version = 3) : m_version(version) {}

    GgufBuilder &addUint32(const QString &key, quint32 value) { return addTyped(key, 4, scalarBytes(value)); }
    GgufBuilder &addString(const QString &key, const QString &value) { return addTyped(key, 8, stringBytes(value)); }
    GgufBuilder &addStringArray(const QString &key, const QStringList &values) {
        QByteArray body = scalarBytes(quint32(8)) + scalarBytes(quint64(values.size()));
        for (const QString &value : values) {
            body += stringBytes(value);
        }
        return addTyped(key, 9, body);
    }
    GgufBuilder &addUint32Array(const QString &key, const QVector<quint32> &values) {
        QByteArray body = scalarBytes(quint32(4)) + scalarBytes(quint64(values.size()));
        for (const quint32 value : values) {
            body += scalarBytes(value);
        }
        return addTyped(key, 9, body);
    }
    GgufBuilder &addRawValue(const QString &key, quint32 type, const QByteArray &body) { return addTyped(key, type, body); }
    GgufBuilder &addTensor(const QString &name, quint64 offset) {
        m_tensors.append({name, offset});
        return *this;
    }

    // Header bytes up to the aligned start of tensor data.
    QByteArray header(quint64 alignment = 32) const {
        QByteArray bytes = QByteArrayLiteral("GGUF") + scalarBytes(m_version) + scalarBytes(quint64(m_tensors.size())) +
                           scalarBytes(quint64(m_keyCount)) + m_metadata;
        for (const auto &tensor : m_tensors) {
            bytes += stringBytes(tensor.first) + scalarBytes(quint32(1)) + scalarBytes(quint64(16)) +
                     scalarBytes(quint32(0)) + scalarBytes(tensor.second);
        }
        while (quint64(bytes.size()) % alignment != 0) {
            bytes += '\0';
        }
        return bytes;
    }

    template <typename T>
    static QByteArray scalarBytes(T value) {
        QByteArray bytes(sizeof(T), '\0');
        std::memcpy(bytes.data(), &value, sizeof(T));
        return bytes;
    }
    static QByteArray stringBytes(const QString &text) {
        const QByteArray utf8 = text.toUtf8();
        return scalarBytes(quint64(utf8.size())) + utf8;
    }

private:
    GgufBuilder &addTyped(const QString &key, quint32 type, const QByteArray &body) {
        m_metadata += stringBytes(key) + scalarBytes(type) + body;
        ++m_keyCount;
        return *this;
    }

    quint32 m_version;
    QByteArray m_metadata;
    int m_keyCount = 0;
    QVector<QPair<QString, quint64>> m_tensors;
};

#include "common/python_json.h"

#include <QJsonParseError>
#include <QStringDecoder>

std::optional<QJsonDocument> python_json::parse(const QByteArray &utf8) {
    if (utf8.startsWith("\xEF\xBB\xBF")) {
        return std::nullopt;
    }
    QStringDecoder decoder(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
    const QString text = decoder.decode(utf8);
    if (decoder.hasError()) {
        return std::nullopt;
    }
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        return std::nullopt;
    }
    return document;
}

QString python_json::quote(const QString &text) {
    QString out = QStringLiteral("\"");
    for (const QChar character : text) {
        const char16_t code = character.unicode();
        switch (code) {
        case u'"': out += QStringLiteral("\\\""); continue;
        case u'\\': out += QStringLiteral("\\\\"); continue;
        case u'\n': out += QStringLiteral("\\n"); continue;
        case u'\r': out += QStringLiteral("\\r"); continue;
        case u'\t': out += QStringLiteral("\\t"); continue;
        case u'\b': out += QStringLiteral("\\b"); continue;
        case u'\f': out += QStringLiteral("\\f"); continue;
        default: break;
        }
        if (code >= 0x20 && code <= 0x7E) {
            out += character;
            continue;
        }
        // QString is UTF-16, so astral characters already arrive as the surrogate pair
        // Python's encoder writes.
        out += QStringLiteral("\\u") + QString::number(code, 16).rightJustified(4, u'0');
    }
    return out + u'"';
}

#include "common/python_text.h"

bool python_text::isSpace(QChar character) {
    const char16_t code = character.unicode();
    return character.isSpace() || (code >= 0x1C && code <= 0x1F);
}

QString python_text::strip(const QString &text) {
    qsizetype begin = 0;
    qsizetype end = text.size();
    while (begin < end && isSpace(text.at(begin))) {
        ++begin;
    }
    while (end > begin && isSpace(text.at(end - 1))) {
        --end;
    }
    return text.mid(begin, end - begin);
}

QStringList python_text::splitWords(const QString &text) {
    QStringList words;
    QString current;
    for (const QChar character : text) {
        if (!isSpace(character)) {
            current += character;
        } else if (!current.isEmpty()) {
            words.append(current);
            current.clear();
        }
    }
    if (!current.isEmpty()) {
        words.append(current);
    }
    return words;
}

QStringList python_text::splitLines(const QString &text) {
    QStringList lines;
    QString current;
    for (qsizetype at = 0; at < text.size(); ++at) {
        const char16_t code = text.at(at).unicode();
        const bool isBreak = code == u'\n' || code == u'\r' || code == 0x0B || code == 0x0C
                             || (code >= 0x1C && code <= 0x1E);
        if (!isBreak) {
            current += text.at(at);
            continue;
        }
        if (code == u'\r' && at + 1 < text.size() && text.at(at + 1) == u'\n') {
            ++at;
        }
        lines.append(current);
        current.clear();
    }
    if (!current.isEmpty()) {
        lines.append(current);
    }
    return lines;
}

#include "launch/extra_args.h"

#include <optional>

namespace {

bool isShlexSpace(QChar character) {
    return character == u' ' || character == u'\t' || character == u'\r' || character == u'\n';
}

bool isQuote(QChar character) {
    return character == u'"' || character == u'\'';
}

QString stripDoubleQuotes(const QString &token) {
    qsizetype begin = 0;
    qsizetype end = token.size();
    while (begin < end && token.at(begin) == u'"') {
        ++begin;
    }
    while (end > begin && token.at(end - 1) == u'"') {
        --end;
    }
    return token.mid(begin, end - begin);
}

// shlex.read_token with posix=False, whitespace_split=True and no comment characters. In this
// mode a token ends at whitespace, at the end of input, or right after its closing quote when
// the token started with a quote; quotes met inside a word are plain characters.
class NonPosixLexer {
public:
    explicit NonPosixLexer(const QString &text) : m_text(text) {}

    // A token, nullopt at the end of input; failure for a quote left open.
    Result<std::optional<QString>> next() {
        QString token;
        QChar state = u' ';
        while (m_position <= m_text.size()) {
            const bool isEnd = m_position == m_text.size();
            const QChar character = isEnd ? QChar() : m_text.at(m_position);
            ++m_position;
            if (isQuote(state)) {
                if (isEnd) {
                    return Result<std::optional<QString>>::failure(
                        QStringLiteral("Extra arguments have a quote that is never closed."));
                }
                token += character;
                if (character == state) {
                    return Result<std::optional<QString>>::success(token);
                }
                continue;
            }
            if (isEnd) {
                break;
            }
            if (isShlexSpace(character)) {
                if (state == u'a') {
                    return Result<std::optional<QString>>::success(token);
                }
                continue;
            }
            token += character;
            if (state == u' ') {
                state = isQuote(character) ? character : QChar(u'a');
            }
        }
        if (state == u'a') {
            return Result<std::optional<QString>>::success(token);
        }
        return Result<std::optional<QString>>::success(std::nullopt);
    }

private:
    const QString &m_text;
    qsizetype m_position = 0;
};

} // namespace

Result<QStringList> ExtraArgs::split(const QString &text) {
    NonPosixLexer lexer(text);
    QStringList tokens;
    while (true) {
        const auto token = lexer.next();
        if (!token.ok()) {
            return Result<QStringList>::failure(token.error());
        }
        if (!token.value()) {
            return Result<QStringList>::success(tokens);
        }
        tokens.append(stripDoubleQuotes(*token.value()));
    }
}

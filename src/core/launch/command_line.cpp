#include "launch/command_line.h"

QString CommandLine::join(const QStringList &arguments) {
    QString result;
    for (const QString &argument : arguments) {
        if (!result.isEmpty()) {
            result += u' ';
        }
        const bool needsQuotes = argument.isEmpty() || argument.contains(u' ') || argument.contains(u'\t');
        if (needsQuotes) {
            result += u'"';
        }
        // Backslashes are literal unless they precede a quote, where each one must be doubled.
        qsizetype pendingBackslashes = 0;
        for (const QChar character : argument) {
            if (character == u'\\') {
                ++pendingBackslashes;
            } else if (character == u'"') {
                result += QString(pendingBackslashes * 2, u'\\') + QStringLiteral("\\\"");
                pendingBackslashes = 0;
            } else {
                result += QString(pendingBackslashes, u'\\') + character;
                pendingBackslashes = 0;
            }
        }
        result += QString(pendingBackslashes, u'\\');
        if (needsQuotes) {
            // a closing quote turns trailing backslashes into escapes, so double them
            result += QString(pendingBackslashes, u'\\') + u'"';
        }
    }
    return result;
}

#pragma once

#include <QByteArray>
#include <QStringList>

// Turns raw llama-server output chunks into lines the way Python's text-mode pipe does
// (universal newlines: "\r\n", "\r" and "\n" all end a line), even when a "\r\n" pair is split
// across two chunks. Lines are decoded as UTF-8 with replacement, without their line end.
class LineSplitter {
public:
    QStringList feed(const QByteArray &chunk);

    // The bytes after the last line end; glm5 builds print progress dots there without a newline.
    const QByteArray &pending() const { return m_pending; }

    // At end of output: the unterminated last line, if any (Python yields it too).
    QStringList finish();

private:
    QByteArray m_pending;
    bool m_isAfterCarriageReturn = false;
};

#pragma once

#include "common/result.h"

#include <QString>
#include <QStringList>

// The Extra args field, tokenized as read_settings does: shlex.split(text, posix=False), then
// strip('"') on every token (FR-3.5, 02 H11). Non-POSIX shlex keeps quotes inside words, does not
// group "a b" in the middle of a word, keeps single quotes, treats backslash and # as ordinary
// characters, accepts an unbalanced quote at the end of a word and rejects one that opens a token.
class ExtraArgs {
public:
    static Result<QStringList> split(const QString &text);
};

#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QString>

#include <optional>

// The two places the reference leans on Python's json module: reading files with
// json.load(open(path, encoding="utf-8")) and building the hot-cache key with json.dumps.
namespace python_json {

// json.load on a file opened as UTF-8 text: invalid UTF-8 and a leading BOM are errors there
// (UnicodeDecodeError / "Unexpected UTF-8 BOM"), so they are errors here too. Only objects and
// arrays are accepted at the top level, which is all the reference ever reads.
std::optional<QJsonDocument> parse(const QByteArray &utf8);

// json.dumps(text) with the default ensure_ascii=True: quotes, backslash escapes and
// lowercase \uXXXX for everything outside printable ASCII (02 H9).
QString quote(const QString &text);

} // namespace python_json

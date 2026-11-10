#pragma once

#include <QString>
#include <QStringList>

// Python str methods whose whitespace rules differ from Qt's: Python counts the ASCII
// separators 0x1C-0x1F as whitespace and splitlines() breaks on more than \r and \n. The
// reference applies them to hot files and server output.
namespace python_text {

// str.isspace() for one character.
bool isSpace(QChar character);

// str.strip()
QString strip(const QString &text);

// str.split() with no separator: runs of whitespace, no empty fields.
QStringList splitWords(const QString &text);

// str.splitlines() for ASCII text: \r\n, \r, \n, \v, \f and 0x1C-0x1E end a line.
QStringList splitLines(const QString &text);

} // namespace python_text
